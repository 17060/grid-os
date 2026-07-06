#include "basic.h"

#include "ai.h"
#include "btc.h"
#include "console.h"
#include "gfs.h"
#include "log.h"
#include "irc.h"
#include "net.h"
#include "program.h"
#include "serial.h"
#include "timer.h"

#include <stddef.h>
#include <stdint.h>

/* ===== GridBASIC interpreter ============================================
 *
 * Tokenize the whole program into a flat token array (with NEWLINE tokens
 * preserving line structure), then walk it with a global cursor. GOTO and
 * GOSUB targets resolve through a line-number -> token-index table built
 * during tokenization. FOR/WHILE/GOSUB frames record token indices to jump
 * back to. IF...THEN executes the remainder of the line inline.
 * ======================================================================= */

typedef enum {
    T_NUM, T_STR, T_ID, T_KW, T_OP, T_NEWLINE, T_EOF
} ttype_t;

typedef enum {
    KW_NONE = 0,
    KW_PRINT, KW_LET, KW_IF, KW_THEN, KW_ELSE,
    KW_FOR, KW_TO, KW_STEP, KW_NEXT,
    KW_WHILE, KW_WEND, KW_REPEAT, KW_UNTIL,
    KW_GOTO, KW_GOSUB, KW_RETURN,
    KW_REM, KW_INPUT, KW_DIM, KW_END, KW_STOP,
    KW_AND, KW_OR, KW_NOT, KW_MOD, KW_DIV
} kw_t;

typedef struct {
    ttype_t type;
    kw_t kw;
    int op;          /* single-char operator or 0 */
    int64_t num;     /* numeric literal, stored as fixed-point (value * SCALE) */
    char text[64];   /* identifier / string literal (unescaped) */
    int slen;        /* string literal length */
    int line_no;     /* source line number this token starts on */
} token_t;

#define MAX_TOKENS 4096
#define MAX_LINES  512
#define MAX_VARS   128
#define MAX_FRAMES 64

/* GridBASIC numbers are stored as signed 64-bit fixed-point with 6 decimal
 * places: a value V is represented as V * SCALE. This avoids all hardware
 * floating-point (the kernel boot stub does not enable CR4.OSFXSR, so SSE/x87
 * instructions silently miscompute in ring 0). All math below is integer
 * arithmetic, using __int128 intermediates for mul/div to avoid overflow. */
#define BASIC_SCALE 1000000LL
typedef int64_t num_t;

typedef struct {
    int is_str;
    num_t n;         /* fixed-point numeric value */
    char s[512];
} value_t;

typedef struct {
    char name[24];
    int is_str;        /* variable's value type */
    int is_array;
    int dim;           /* array upper bound (inclusive), 0 if scalar */
    value_t *cells;    /* dim+1 cells if array */
    value_t scalar;
} var_t;

typedef struct {
    int tok_index;     /* where to resume */
    int var_index;     /* FOR control variable */
    num_t end_val;
    num_t step_val;
} for_frame_t;

typedef struct {
    int tok_index;     /* return point */
} gosub_frame_t;

static token_t g_tokens[MAX_TOKENS];
static int g_ntok;
static volatile int g_cur;      /* cursor into tokens — volatile so the
                                 * cross-compiler always reloads it after
                                 * inlined match_op/eval_expr calls */

static int g_labels[MAX_LINES];   /* line number */
static int g_label_tok[MAX_LINES];/* token index of that line */
static int g_nlabels;

static var_t g_vars[MAX_VARS];
static int g_nvars;

static void set_error(const char *msg);  /* forward */

/* Bump allocator for array cell storage (no malloc in the kernel). */
static value_t g_array_pool[2048];
static int g_pool_used;

static value_t *alloc_cells(int n) {
    if (n < 1) n = 1;
    if (g_pool_used + n > (int)(sizeof(g_array_pool) / sizeof(g_array_pool[0]))) {
        set_error("ARRAY: pool exhausted");
        return 0;
    }
    value_t *p = &g_array_pool[g_pool_used];
    g_pool_used += n;
    for (int i = 0; i < n; ++i) { p[i].is_str = 0; p[i].n = 0; p[i].s[0] = '\0'; }
    return p;
}

static for_frame_t g_for_stack[MAX_FRAMES];
static int g_for_sp;
static gosub_frame_t g_gosub_stack[MAX_FRAMES];
static int g_gosub_sp;

static int g_running;
static int g_error;
static char g_errmsg[96];

/* ---- small helpers ---- */

static int is_alpha(int c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'; }
static int is_digit(int c) { return c >= '0' && c <= '9'; }
static int is_alnum(int c) { return is_alpha(c) || is_digit(c); }
static int to_upper(int c) { return (c >= 'a' && c <= 'z') ? c - 32 : c; }
static int name_is_str(const char *name) {
    if (!name || !name[0]) {
        return 0;
    }
    const char *p = name;
    while (*p) {
        p++;
    }
    return p[-1] == '$';
}

/* forward declarations — helpers defined later but used by the evaluator */
static int str_contains(const char *s, char ch);
static int str_len(const char *s);
static int str_cmp(const char *a, const char *b);
static void num_to_string(num_t v, char *out, size_t cap);
static num_t sqrt_local(num_t x);
static num_t pow_local(num_t base, num_t exp);
static uint32_t rnd_local(void);

static void set_error(const char *msg) {
    if (!g_error) {
        g_error = 1;
        for (size_t i = 0; i < sizeof(g_errmsg) - 1 && msg[i]; ++i) {
            g_errmsg[i] = msg[i];
            g_errmsg[i + 1] = '\0';
        }
    }
    g_running = 0;
}

static int strequal(const char *a, const char *b) {
    while (*a && *b) {
        if (to_upper(*a) != to_upper(*b)) return 0;
        a++; b++;
    }
    return *a == *b;
}

static num_t fmod_local(num_t a, num_t b) {
    if (b == 0) return 0;
    num_t q = a / b;
    return a - q * b;
}

static num_t fdiv_local(num_t a, num_t b) {
    if (b == 0) return 0;
    /* a/b in fixed-point: (a * SCALE) / b */
    return (num_t)((__int128)a * BASIC_SCALE / b);
}

/* ---- tokenizer ---- */

static kw_t match_keyword(const char *w) {
    if (strequal(w, "PRINT") || strequal(w, "?")) return KW_PRINT;
    if (strequal(w, "LET")) return KW_LET;
    if (strequal(w, "IF")) return KW_IF;
    if (strequal(w, "THEN")) return KW_THEN;
    if (strequal(w, "ELSE")) return KW_ELSE;
    if (strequal(w, "FOR")) return KW_FOR;
    if (strequal(w, "TO")) return KW_TO;
    if (strequal(w, "STEP")) return KW_STEP;
    if (strequal(w, "NEXT")) return KW_NEXT;
    if (strequal(w, "WHILE")) return KW_WHILE;
    if (strequal(w, "WEND")) return KW_WEND;
    if (strequal(w, "REPEAT")) return KW_REPEAT;
    if (strequal(w, "UNTIL")) return KW_UNTIL;
    if (strequal(w, "GOTO")) return KW_GOTO;
    if (strequal(w, "GOSUB")) return KW_GOSUB;
    if (strequal(w, "RETURN")) return KW_RETURN;
    if (strequal(w, "REM")) return KW_REM;
    if (strequal(w, "INPUT")) return KW_INPUT;
    if (strequal(w, "DIM")) return KW_DIM;
    if (strequal(w, "END")) return KW_END;
    if (strequal(w, "STOP")) return KW_STOP;
    if (strequal(w, "AND")) return KW_AND;
    if (strequal(w, "OR")) return KW_OR;
    if (strequal(w, "NOT")) return KW_NOT;
    if (strequal(w, "MOD")) return KW_MOD;
    if (strequal(w, "DIV")) return KW_DIV;
    return KW_NONE;
}

static int g_tok_overflow;

static int push_tok(token_t *t) {
    if (g_ntok >= MAX_TOKENS) {
        g_tok_overflow = 1;
        return -1;
    }
    g_tokens[g_ntok] = *t;
    g_ntok++;
    return 0;
}

static int parse_number(const char *src, size_t *i, num_t *out) {
    size_t start = *i;
    while (is_digit(src[*i])) (*i)++;
    int has_dot = 0;
    if (src[*i] == '.') { has_dot = 1; (*i)++; while (is_digit(src[*i])) (*i)++; }
    if (src[*i] == 'E' || src[*i] == 'e') {
        size_t save = *i;
        (*i)++;
        if (src[*i] == '+' || src[*i] == '-') (*i)++;
        if (!is_digit(src[*i])) { *i = save; }
        else { while (is_digit(src[*i])) (*i)++; }
    }
    (void)has_dot;
    /* convert to fixed-point (value * SCALE) using integer arithmetic */
    num_t whole = 0;
    size_t k = start;
    while (k < *i && src[k] != '.' && src[k] != 'E' && src[k] != 'e') {
        whole = whole * 10 + (src[k] - '0'); k++;
    }
    num_t val = whole * BASIC_SCALE;
    if (src[k] == '.') {
        k++;
        num_t frac = 0; num_t div = 1;
        int digits = 0;
        while (k < *i && src[k] != 'E' && src[k] != 'e' && digits < 6) {
            frac = frac * 10 + (src[k] - '0'); div *= 10; k++; digits++;
        }
        /* skip remaining fractional digits beyond 6 */
        while (k < *i && src[k] != 'E' && src[k] != 'e') k++;
        val += (div > 0) ? (frac * BASIC_SCALE / div) : 0;
    }
    /* exponent (integer power of 10) */
    if (src[k] == 'E' || src[k] == 'e') {
        k++;
        int neg = 0;
        if (src[k] == '+') k++; else if (src[k] == '-') { neg = 1; k++; }
        int e = 0;
        while (k < *i && is_digit(src[k])) { e = e * 10 + (src[k] - '0'); k++; }
        num_t m = BASIC_SCALE;
        for (int j = 0; j < e; ++j) m *= 10;
        val = neg ? (num_t)((__int128)val * BASIC_SCALE / m) : (num_t)((__int128)val * m / BASIC_SCALE);
    }
    *out = val;
    return 1;
}

static int tokenize(const char *src) {
    g_ntok = 0;
    g_nlabels = 0;
    g_tok_overflow = 0;
    size_t i = 0;
    int line_no = 0;
    int at_line_start = 1;

    while (src[i]) {
        char c = src[i];
        if (c == '\n') {
            token_t t = {0}; t.type = T_NEWLINE; t.line_no = line_no;
            push_tok(&t);
            line_no++;
            i++;
            at_line_start = 1;
            continue;
        }
        if (c == ' ' || c == '\t' || c == '\r') { i++; continue; }

        /* line-number label at start of line */
        if (at_line_start && is_digit(c)) {
            num_t v = 0;
            size_t before = i;
            parse_number(src, &i, &v);
            if (g_nlabels < MAX_LINES) {
                g_labels[g_nlabels] = (int)(v / BASIC_SCALE);
                g_label_tok[g_nlabels] = g_ntok;
                g_nlabels++;
            }
            (void)before;
            at_line_start = 0;
            continue;
        }
        at_line_start = 0;

        /* comment REM ... -> consume to end of line */
        if (c == '\'') {
            while (src[i] && src[i] != '\n') i++;
            continue;
        }

        /* string literal */
        if (c == '"') {
            i++;
            token_t t = {0}; t.type = T_STR; t.line_no = line_no;
            int n = 0;
            while (src[i] && src[i] != '"' && src[i] != '\n') {
                if (n < (int)sizeof(t.text) - 1) t.text[n++] = src[i];
                i++;
            }
            t.text[n] = '\0';
            t.slen = n;
            if (src[i] == '"') i++;
            push_tok(&t);
            continue;
        }

        /* number */
        if (is_digit(c) || (c == '.' && is_digit(src[i+1]))) {
            num_t v = 0;
            parse_number(src, &i, &v);
            token_t t = {0}; t.type = T_NUM; t.num = v; t.line_no = line_no;
            push_tok(&t);
            continue;
        }

        /* identifier / keyword (supports trailing $ and %, and . for GRID.*) */
        if (is_alpha(c)) {
            token_t t = {0}; t.type = T_ID; t.line_no = line_no;
            int n = 0;
            while (is_alnum(src[i])) {
                if (n < (int)sizeof(t.text) - 4) t.text[n++] = to_upper(src[i]);
                i++;
            }
            if (src[i] == '$' || src[i] == '%') {
                t.text[n++] = src[i]; i++;
            } else {
                while (src[i] == '.') {
                    if (n < (int)sizeof(t.text) - 4) t.text[n++] = '.';
                    i++;
                    while (is_alnum(src[i])) {
                        if (n < (int)sizeof(t.text) - 2) t.text[n++] = to_upper(src[i]);
                        i++;
                    }
                }
                if (src[i] == '$') { t.text[n++] = '$'; i++; }
            }
            t.text[n] = '\0';
            /* keyword? (only single-word identifiers, no dot) */
            kw_t kw = match_keyword(t.text);
            if (kw != KW_NONE && !str_contains(t.text, '.')) {
                t.type = T_KW; t.kw = kw;
                if (kw == KW_REM) {
                    while (src[i] && src[i] != '\n') i++;
                }
            }
            push_tok(&t);
            continue;
        }

        /* two-char operators */
        if ((c == '<' && (src[i+1] == '>' || src[i+1] == '=')) ||
            (c == '>' && src[i+1] == '=') ||
            (c == ':' && src[i+1] == '=')) {
            token_t t = {0}; t.type = T_OP; t.line_no = line_no;
            if (c == '<' && src[i+1] == '>') t.op = '#';
            else if (c == '<' && src[i+1] == '=') t.op = 'L';
            else if (c == '>' && src[i+1] == '=') t.op = 'G';
            else t.op = 'E'; /* := assignment, treat as = */
            t.text[0] = t.op; t.text[1] = '\0';
            push_tok(&t);
            i += 2;
            continue;
        }

        /* single-char operators / punctuation */
        {
            token_t t = {0}; t.type = T_OP; t.op = c; t.line_no = line_no;
            t.text[0] = c; t.text[1] = '\0';
            push_tok(&t);
            i++;
            continue;
        }
    }
    if (g_tok_overflow) {
        return -1;
    }
    token_t t = {0}; t.type = T_EOF; t.line_no = line_no;
    push_tok(&t);
    return 0;
}

/* ---- variables ---- */

static var_t *find_var(const char *name) {
    for (int i = 0; i < g_nvars; ++i) {
        if (strequal(g_vars[i].name, name)) return &g_vars[i];
    }
    return 0;
}

static var_t *get_var(const char *name, int want_str) {
    var_t *v = find_var(name);
    if (v) return v;
    if (g_nvars >= MAX_VARS) { set_error("VAR: too many variables"); return 0; }
    v = &g_vars[g_nvars++];
    size_t n = 0;
    while (name[n] && n < sizeof(v->name) - 1) { v->name[n] = name[n]; n++; }
    v->name[n] = '\0';
    v->is_str = want_str;
    v->is_array = 0;
    v->dim = 0;
    v->cells = 0;
    v->scalar.is_str = want_str;
    v->scalar.n = 0;
    v->scalar.s[0] = '\0';
    return v;
}

static value_t *var_cell(var_t *v, int idx) {
    if (!v->is_array) return &v->scalar;
    if (idx < 0 || idx > v->dim) { set_error("ARRAY: index out of range"); return 0; }
    return &v->cells[idx];
}

static void reset_state(void) {
    g_nvars = 0;
    g_for_sp = 0;
    g_gosub_sp = 0;
    g_pool_used = 0;
    g_cur = 0;
    g_error = 0;
    g_errmsg[0] = '\0';
    g_running = 1;
}

/* ---- expression evaluator (recursive descent) ---- */

static value_t eval_expr(void);

static token_t *cur(void) { return &g_tokens[g_cur]; }
static void advance(void) { if (g_cur < g_ntok - 1) g_cur++; }
static int match_op(char op) {
    if (cur()->type == T_OP && cur()->op == op) { advance(); return 1; }
    return 0;
}
static int match_kw(kw_t kw) {
    if (cur()->type == T_KW && cur()->kw == kw) { advance(); return 1; }
    return 0;
}

static void num_to_string(num_t raw, char *out, size_t cap) {
    /* raw is value * SCALE (fixed-point). Print integer part + trimmed fraction. */
    int neg = raw < 0;
    num_t a = neg ? -raw : raw;
    num_t whole = a / BASIC_SCALE;
    num_t frac  = a % BASIC_SCALE;   /* > 0 iff there is a fractional part */
    char tmp[32];
    int t = 0;
    if (whole == 0) tmp[t++] = '0';
    while (whole > 0) { tmp[t++] = (char)('0' + (int)(whole % 10)); whole /= 10; }
    size_t p = 0;
    if (neg && p < cap - 1) out[p++] = '-';
    while (t > 0 && p < cap - 1) out[p++] = tmp[--t];
    if (frac > 0) {
        if (p < cap - 1) out[p++] = '.';
        /* Build the 6 fractional digits, least-significant first:
         * six[0] = 10^-6 place, ..., six[5] = 10^-1 place.
         * Preserve leading zeros (e.g. 0.05 -> "05") but trim trailing zeros. */
        char six[8]; int six_n = 6;
        num_t f6 = a % BASIC_SCALE;
        for (int d = 0; d < 6; ++d) { six[d] = (char)('0' + (int)(f6 % 10)); f6 /= 10; }
        /* lo = least-significant non-zero digit index (trailing zeros below it) */
        int lo = 0;
        while (lo < six_n && six[lo] == '0') lo++;
        if (lo >= six_n) lo = six_n - 1; /* all zeros (shouldn't happen, frac>0) */
        /* print from most-significant (index 5) down to lo */
        for (int d = 5; d >= lo && p < cap - 1; --d) out[p++] = six[d];
    }
    out[p] = '\0';
}

static value_t make_num(num_t v) {
    value_t r; r.is_str = 0; r.n = v; r.s[0] = '\0'; return r;
}
static value_t make_str(const char *s) {
    value_t r; r.is_str = 1; r.n = 0; size_t k = 0;
    while (s[k] && k < sizeof(r.s) - 1) { r.s[k] = s[k]; k++; }
    r.s[k] = '\0'; return r;
}

/* convert a string's leading numeric portion to fixed-point */
static num_t parse_str_to_num(const char *s) {
    int neg = 0;
    if (*s == '-') { neg = 1; s++; }
    else if (*s == '+') { s++; }
    num_t whole = 0;
    while (*s >= '0' && *s <= '9') { whole = whole * 10 + (*s - '0'); s++; }
    num_t val = whole * BASIC_SCALE;
    if (*s == '.') {
        s++;
        num_t frac = 0; num_t div = 1; int digits = 0;
        while (*s >= '0' && *s <= '9' && digits < 6) {
            frac = frac * 10 + (*s - '0'); div *= 10; s++; digits++;
        }
        val += (div > 0) ? (frac * BASIC_SCALE / div) : 0;
    }
    return neg ? -val : val;
}

static num_t to_num(const value_t *v) {
    if (v->is_str) return parse_str_to_num(v->s);
    return v->n;
}

static int to_bool(const value_t *v) {
    if (v->is_str) return v->s[0] != '\0';
    return v->n != 0;
}

/* built-in functions: FUNC(args) — we are sitting at the function name T_ID */
static value_t eval_builtin(const char *name, int argc, value_t *argv) {
    if (strequal(name, "ABS"))     return make_num(argc > 0 ? (to_num(&argv[0]) < 0 ? -to_num(&argv[0]) : to_num(&argv[0])) : 0);
    if (strequal(name, "INT"))     { num_t x = argc>0?to_num(&argv[0]):0; num_t ip = x / BASIC_SCALE; return make_num(ip * BASIC_SCALE); }
    if (strequal(name, "SGN"))     { num_t x = argc>0?to_num(&argv[0]):0; return make_num(x<0?-BASIC_SCALE:(x>0?BASIC_SCALE:0)); }
    if (strequal(name, "SQR"))     return make_num(argc>0 && to_num(&argv[0])>=0 ? sqrt_local(to_num(&argv[0])) : 0);
    if (strequal(name, "RND"))     { if (argc == 0) { return make_num((num_t)(rnd_local() & 0xFFFFFFu) * BASIC_SCALE / (num_t)0x1000000u); } num_t x = to_num(&argv[0]); int m = (int)(x / BASIC_SCALE); if (m <= 0) m = 1; return make_num((num_t)(rnd_local() % (uint32_t)m) * BASIC_SCALE); }
    if (strequal(name, "LEN"))     return make_num((argc>0 && argv[0].is_str) ? (num_t)str_len(argv[0].s) * BASIC_SCALE : 0);
    if (strequal(name, "VAL"))     return make_num(argc>0?to_num(&argv[0]):0);
    if (strequal(name, "ASC"))     return make_num(argc>0&&argv[0].is_str && argv[0].s[0]?(num_t)(uint8_t)argv[0].s[0]*BASIC_SCALE:0);
    if (strequal(name, "CHR$"))    { char b[2]; b[0]=(char)(argc>0?(int)(to_num(&argv[0])/BASIC_SCALE):0); b[1]='\0'; return make_str(b); }
    if (strequal(name, "STR$"))    { char b[32]; num_to_string(argc>0?to_num(&argv[0]):0, b, sizeof(b)); return make_str(b); }
    if (strequal(name, "UPPER$"))  { if(!(argc>0&&argv[0].is_str)) return make_str(""); char b[160]; size_t k=0; const char*s=argv[0].s; while(*s&&k<sizeof(b)-1){b[k++]=to_upper(*s);s++;} b[k]='\0'; return make_str(b); }
    if (strequal(name, "LOWER$"))  { if(!(argc>0&&argv[0].is_str)) return make_str(""); char b[160]; size_t k=0; const char*s=argv[0].s; while(*s&&k<sizeof(b)-1){b[k++]=(*s>='A'&&*s<='Z')?*s+32:*s;s++;} b[k]='\0'; return make_str(b); }
    if (strequal(name, "LEFT$"))   { if(!(argc>0&&argv[0].is_str)) return make_str(""); int n=(int)(argc>1?to_num(&argv[1])/BASIC_SCALE:0); if(n<0)n=0; char b[160]; size_t k=0; const char*s=argv[0].s; while(*s&&k<(size_t)n&&k<sizeof(b)-1){b[k++]=*s++;} b[k]='\0'; return make_str(b); }
    if (strequal(name, "RIGHT$"))  { if(!(argc>0&&argv[0].is_str)) return make_str(""); int n=(int)(argc>1?to_num(&argv[1])/BASIC_SCALE:0); int len=str_len(argv[0].s); if(n>len)n=len; if(n<0)n=0; const char*s=argv[0].s+len-n; return make_str(s); }
    if (strequal(name, "MID$"))    { if(!(argc>0&&argv[0].is_str)) return make_str(""); int start=(int)(argc>1?to_num(&argv[1])/BASIC_SCALE:1); int n=(int)(argc>2?to_num(&argv[2])/BASIC_SCALE:32000); if(start<1)start=1; const char*s=argv[0].s+start-1; if(s<argv[0].s)s=argv[0].s; char b[160]; size_t k=0; while(*s&&k<(size_t)n&&k<sizeof(b)-1){b[k++]=*s++;} b[k]='\0'; return make_str(b); }
    if (strequal(name, "PI"))      return make_num(3141593LL);  /* 3.141593 * SCALE */
    /* GRID.* bindings exposed as functions */
    if (strequal(name, "GRID.TIME"))    return make_num((num_t)timer_ticks() * BASIC_SCALE);
    if (strequal(name, "GRID.RND"))     { int m=(int)(argc>0?to_num(&argv[0])/BASIC_SCALE:100); if(m<=0)m=100; return make_num((num_t)(rnd_local()%m) * BASIC_SCALE); }
    if (strequal(name, "GRID.PING"))    { if(!(argc>0&&argv[0].is_str)) return make_num(0); uint32_t ip; if(net_parse_ip(argv[0].s,&ip)!=0) return make_num(0); return make_num(net_ping(ip) ? BASIC_SCALE : 0); }
    if (strequal(name, "GRID.SERIAL.READ$")) { char b[128]; size_t got=serial_read_line(b,sizeof(b),200000); (void)got; return make_str(b); }
    if (strequal(name, "GRID.STATUS$")) { return make_str("Grid OS 6.2 — GridBASIC online"); }
    if (strequal(name, "GRID.CAP"))     { return make_num(BASIC_SCALE); }
    if (strequal(name, "GRID.AI.ASK$") || strequal(name, "GRID.AI.COMPLETE$")) {
        if (!(argc > 0 && argv[0].is_str)) { return make_str(""); }
        char b[512]; ai_ask(argv[0].s, b, sizeof(b)); return make_str(b);
    }
    if (strequal(name, "GRID.AI.EXPLAIN$")) {
        if (!(argc > 0 && argv[0].is_str)) { return make_str(""); }
        char b[512]; ai_explain(argv[0].s, b, sizeof(b)); return make_str(b);
    }
    if (strequal(name, "GRID.AI.FIX$")) {
        if (!(argc > 0 && argv[0].is_str)) { return make_str(""); }
        char b[512]; ai_fix(argv[0].s, b, sizeof(b)); return make_str(b);
    }
    if (strequal(name, "GRID.AI.MODELS$")) {
        char b[160]; ai_models(b, sizeof(b)); return make_str(b);
    }
    if (strequal(name, "GRID.IRC.READ$")) {
        char b[IRC_LINE_MAX]; irc_read(b, sizeof(b)); return make_str(b);
    }
    if (strequal(name, "GRID.IRC.STATUS$")) {
        char b[IRC_LINE_MAX]; irc_status(b, sizeof(b)); return make_str(b);
    }
    if (strequal(name, "GRID.IRC.CONNECT$")) {
        if (!(argc >= 3 && argv[0].is_str && argv[2].is_str)) { return make_str("error: bad args"); }
        uint16_t port = (uint16_t)(to_num(&argv[1]) / BASIC_SCALE);
        if (port == 0) { return make_str("error: bad port"); }
        return make_str(irc_connect(argv[0].s, port, argv[2].s) == 0 ? "ok" : "error: connect failed");
    }
    if (strequal(name, "GRID.BTC.CALL$")) {
        if (!(argc >= 1 && argv[0].is_str)) { return make_str(""); }
        const char *params = (argc >= 2 && argv[1].is_str) ? argv[1].s : "";
        char b[BTC_RESP_MAX]; btc_call(argv[0].s, params, b, sizeof(b)); return make_str(b);
    }
    if (strequal(name, "GRID.BTC.INFO$") || strequal(name, "GRID.BTC.BLOCKCHAIN$")) {
        char b[BTC_RESP_MAX]; btc_blockchain(b, sizeof(b)); return make_str(b);
    }
    if (strequal(name, "GRID.BTC.NETWORK$")) {
        char b[BTC_RESP_MAX]; btc_network(b, sizeof(b)); return make_str(b);
    }
    if (strequal(name, "GRID.BTC.WALLET$")) {
        char b[BTC_RESP_MAX]; btc_wallet(b, sizeof(b)); return make_str(b);
    }
    if (strequal(name, "GRID.BTC.BALANCE$")) {
        char b[BTC_RESP_MAX]; btc_balance(b, sizeof(b)); return make_str(b);
    }
    if (strequal(name, "GRID.BTC.ADDRESS$")) {
        const char *label = (argc >= 1 && argv[0].is_str) ? argv[0].s : "";
        char b[BTC_RESP_MAX]; btc_address(label, b, sizeof(b)); return make_str(b);
    }
    if (strequal(name, "GRID.BTC.HELP$")) {
        char b[BTC_RESP_MAX]; btc_help(b, sizeof(b)); return make_str(b);
    }
    if (strequal(name, "GRID.BTC.STATUS$")) {
        char b[BTC_RESP_MAX]; btc_status(b, sizeof(b)); return make_str(b);
    }
    set_error("FUNC: unknown function");
    return make_num(0);
}

static int is_builtin_name(const char *name) {
    if (strequal(name, "ABS") || strequal(name, "INT") || strequal(name, "SGN") ||
        strequal(name, "SQR") || strequal(name, "RND") || strequal(name, "LEN") ||
        strequal(name, "VAL") || strequal(name, "ASC") || strequal(name, "CHR$") ||
        strequal(name, "STR$") || strequal(name, "UPPER$") || strequal(name, "LOWER$") ||
        strequal(name, "LEFT$") || strequal(name, "RIGHT$") || strequal(name, "MID$") ||
        strequal(name, "PI") ||
        strequal(name, "GRID.TIME") || strequal(name, "GRID.RND") ||
        strequal(name, "GRID.PING") || strequal(name, "GRID.SERIAL.READ$") ||
        strequal(name, "GRID.STATUS$") || strequal(name, "GRID.CAP") ||
        strequal(name, "GRID.AI.ASK$") || strequal(name, "GRID.AI.COMPLETE$") ||
        strequal(name, "GRID.AI.EXPLAIN$") || strequal(name, "GRID.AI.FIX$") ||
        strequal(name, "GRID.AI.MODELS$") ||
        strequal(name, "GRID.IRC.READ$") || strequal(name, "GRID.IRC.STATUS$") ||
        strequal(name, "GRID.IRC.CONNECT$") ||
        strequal(name, "GRID.BTC.CALL$") || strequal(name, "GRID.BTC.INFO$") ||
        strequal(name, "GRID.BTC.BLOCKCHAIN$") || strequal(name, "GRID.BTC.NETWORK$") ||
        strequal(name, "GRID.BTC.WALLET$") || strequal(name, "GRID.BTC.BALANCE$") ||
        strequal(name, "GRID.BTC.ADDRESS$") || strequal(name, "GRID.BTC.HELP$") ||
        strequal(name, "GRID.BTC.STATUS$")) {
        return 1;
    }
    return 0;
}

static value_t eval_primary(void) {
    token_t *t = cur();
    if (t->type == T_NUM) { advance(); return make_num(t->num); }
    if (t->type == T_STR) { advance(); return make_str(t->text); }
    if (match_op('(')) {
        value_t v = eval_expr();
        match_op(')');
        return v;
    }
    if (match_op('-')) { value_t v = eval_primary(); return make_num(-to_num(&v)); }
    if (match_op('+')) { return eval_primary(); }
    if (match_kw(KW_NOT)) { value_t v = eval_primary(); return make_num(to_bool(&v) ? 0 : BASIC_SCALE); }

    if (t->type == T_ID) {
        char name[64];
        size_t k = 0; while (t->text[k] && k < sizeof(name)-1) { name[k] = t->text[k]; k++; } name[k] = '\0';
        int builtin = is_builtin_name(name);
        advance();
        /* function call: builtin names, or any name immediately followed by '('
         * that is NOT an array (arrays are non-builtin identifiers). */
        if (builtin) {
            value_t argv[8]; int argc = 0;
            if (cur()->type == T_OP && cur()->op == '(') {
                advance();
                if (!(cur()->type == T_OP && cur()->op == ')')) {
                    do {
                        if (argc < 8) argv[argc++] = eval_expr();
                    } while (match_op(','));
                }
                match_op(')');
            }
            return eval_builtin(name, argc, argv);
        }
        /* variable or array access */
        int want_str = name_is_str(name);
        var_t *v = get_var(name, want_str);
        if (!v) return make_num(0);
        if (cur()->type == T_OP && cur()->op == '(') {
            advance();
            value_t idx = eval_expr();
            match_op(')');
            value_t *cell = var_cell(v, (int)(to_num(&idx) / BASIC_SCALE));
            if (!cell) return make_num(0);
            return *cell;
        }
        return v->scalar;
    }
    set_error("EXPR: unexpected token");
    return make_num(0);
}

static value_t eval_pow(void) {
    value_t lhs = eval_primary();
    while (cur()->type == T_OP && cur()->op == '^') {
        advance();
        value_t rhs = eval_pow();
        num_t r = pow_local(to_num(&lhs), to_num(&rhs));
        lhs = make_num(r);
    }
    return lhs;
}

static value_t eval_unary(void) {
    return eval_pow();
}

static value_t eval_mul(void) {
    value_t lhs = eval_unary();
    for (;;) {
        if (match_op('*')) { value_t r = eval_unary(); lhs = make_num((num_t)((__int128)to_num(&lhs) * to_num(&r) / BASIC_SCALE)); continue; }
        if (match_op('/')) { value_t r = eval_unary(); lhs = make_num(fdiv_local(to_num(&lhs), to_num(&r))); continue; }
        if (match_kw(KW_MOD)) { value_t r = eval_unary(); lhs = make_num(fmod_local(to_num(&lhs), to_num(&r))); continue; }
        if (match_kw(KW_DIV)) { value_t r = eval_unary(); num_t a=to_num(&lhs), b=to_num(&r); lhs = make_num(b!=0 ? (a / b) * BASIC_SCALE : 0); continue; }
        break;
    }
    return lhs;
}

static value_t eval_add(void) {
    value_t lhs = eval_mul();
    for (;;) {
        if (match_op('+')) {
            value_t r = eval_mul();
            if (lhs.is_str || r.is_str) {
                char lb[160]; if (lhs.is_str) { size_t j=0; while(lhs.s[j]&&j<sizeof(lb)-1){ lb[j]=lhs.s[j]; j++; } lb[j]='\0'; } else num_to_string(lhs.n, lb, sizeof(lb));
                char rb[160]; if (r.is_str)   { size_t j=0; while(r.s[j]&&j<sizeof(rb)-1){ rb[j]=r.s[j]; j++; }   rb[j]='\0'; } else num_to_string(r.n, rb, sizeof(rb));
                char out[320]; size_t p=0; const char*x=lb; while(*x&&p<sizeof(out)-1) out[p++]=*x++; x=rb; while(*x&&p<sizeof(out)-1) out[p++]=*x++; out[p]='\0';
                lhs = make_str(out);
            } else {
                lhs = make_num(to_num(&lhs) + to_num(&r));
            }
            continue;
        }
        if (match_op('-')) { value_t r = eval_mul(); lhs = make_num(to_num(&lhs) - to_num(&r)); continue; }
        break;
    }
    return lhs;
}

static value_t eval_cmp(void) {
    value_t lhs = eval_add();
    for (;;) {
        token_t *t = cur();
        if (t->type == T_OP && (t->op == '=' || t->op == '<' || t->op == '>' || t->op == '#' || t->op == 'L' || t->op == 'G')) {
            advance();
            value_t r = eval_add();
            int res;
            if (lhs.is_str || r.is_str) {
                int c = str_cmp(lhs.is_str ? lhs.s : "", r.is_str ? r.s : "");
                if (t->op == '=') res = (c == 0);
                else if (t->op == '#') res = (c != 0);
                else if (t->op == '<') res = (c < 0);
                else if (t->op == '>') res = (c > 0);
                else if (t->op == 'L') res = (c <= 0);
                else res = (c >= 0);
            } else {
                num_t a = lhs.n, b = r.n;
                if (t->op == '=') res = (a == b);
                else if (t->op == '#') res = (a != b);
                else if (t->op == '<') res = (a < b);
                else if (t->op == '>') res = (a > b);
                else if (t->op == 'L') res = (a <= b);
                else res = (a >= b);
            }
            lhs = make_num(res ? BASIC_SCALE : 0);
            continue;
        }
        break;
    }
    return lhs;
}

static value_t eval_not(void) {
    if (match_kw(KW_NOT)) { value_t v = eval_not(); return make_num(to_bool(&v) ? 0 : BASIC_SCALE); }
    return eval_cmp();
}

static value_t eval_and(void) {
    value_t lhs = eval_not();
    while (match_kw(KW_AND)) { value_t r = eval_not(); lhs = make_num(to_bool(&lhs) && to_bool(&r) ? BASIC_SCALE : 0); }
    return lhs;
}

static value_t eval_or(void) {
    value_t lhs = eval_and();
    while (match_kw(KW_OR)) { value_t r = eval_and(); lhs = make_num(to_bool(&lhs) || to_bool(&r) ? BASIC_SCALE : 0); }
    return lhs;
}

static value_t eval_expr(void) {
    return eval_or();
}

/* ---- statements ---- */

static void print_value(const value_t *v, int trailing_newline, int *column) {
    if (v->is_str) {
        console_write(v->s);
        *column += str_len(v->s);
    } else {
        char b[32]; num_to_string(v->n, b, sizeof(b));
        console_write(b);
        *column += str_len(b);
    }
    if (trailing_newline) console_write_line("");
}

static void exec_print(void) {
    int column = 0;
    int last_was_sep = 0;
    int suppress_nl = 0;
    while (cur()->type != T_NEWLINE && cur()->type != T_EOF &&
           !(cur()->type == T_OP && cur()->op == ':') &&
           !(cur()->type == T_KW && (cur()->kw == KW_ELSE || cur()->kw == KW_END || cur()->kw == KW_STOP))) {
        if (match_op(';')) { last_was_sep = 1; suppress_nl = 1; continue; }
        if (match_op(',')) {
            /* tab to next multiple of 14 */
            int target = (column / 14 + 1) * 14;
            while (column < target) { console_write_char(' '); column++; }
            last_was_sep = 1;
            continue;
        }
        value_t v = eval_expr();
        if (g_error) return;   /* bad expression: bail instead of looping forever */
        print_value(&v, 0, &column);
        last_was_sep = 0;
        suppress_nl = 0;
    }
    if (!suppress_nl && !last_was_sep) console_write_line("");
    (void)last_was_sep;
}

/* assign to a variable (possibly array element). Cursor is at the variable name. */
static void exec_assign(void) {
    char name[64];
    token_t *t = cur();
    size_t k = 0; while (t->text[k] && k < sizeof(name)-1) { name[k] = t->text[k]; k++; } name[k] = '\0';
    advance();
    int want_str = name_is_str(name);
    var_t *v = get_var(name, want_str);
    if (!v) return;
    value_t *cell = &v->scalar;
    if (cur()->type == T_OP && cur()->op == '(') {
        advance();
        value_t idx = eval_expr();
        match_op(')');
        if (!v->is_array) {
            /* auto-dim to idx */
            int d = (int)(to_num(&idx) / BASIC_SCALE);
            if (d < 0) d = 0;
            v->is_array = 1; v->dim = d;
            v->cells = alloc_cells(d + 1);
        }
        cell = var_cell(v, (int)(to_num(&idx) / BASIC_SCALE));
        if (!cell) return;
    }
    if (!match_op('=') && !match_op('E')) { set_error("LET: expected ="); return; }
    value_t val = eval_expr();
    if (g_error) return;
    if (cell->is_str || v->is_str) {
        if (val.is_str) *cell = val;
        else { char b[32]; num_to_string(val.n, b, sizeof(b)); *cell = make_str(b); }
    } else {
        cell->is_str = 0;
        cell->n = to_num(&val);
    }
}

static void exec_dim(void) {
    while (cur()->type == T_ID) {
        char name[64];
        size_t k = 0; while (cur()->text[k] && k < sizeof(name)-1) { name[k] = cur()->text[k]; k++; } name[k] = '\0';
        int want_str = name_is_str(name);
        advance();
        int dim = 0;
        if (match_op('(')) {
            value_t d = eval_expr();
            dim = (int)(to_num(&d) / BASIC_SCALE);
            match_op(')');
        }
        var_t *v = get_var(name, want_str);
        if (v) {
            v->is_array = 1; v->dim = dim;
            v->cells = alloc_cells(dim + 1);
        }
        if (!match_op(',')) break;
    }
}

static void exec_input(void) {
    /* INPUT ["prompt";] var [, var ...] */
    if (cur()->type == T_STR) {
        console_write(cur()->text);
        advance();
        if (match_op(';') || match_op(',')) { /* ok */ }
    }
    while (cur()->type == T_ID) {
        char name[64];
        size_t k = 0; while (cur()->text[k] && k < sizeof(name)-1) { name[k] = cur()->text[k]; k++; } name[k] = '\0';
        int want_str = name_is_str(name);
        advance();
        var_t *v = get_var(name, want_str);
        console_write("? ");
        char buf[160];
        console_read_line(buf, sizeof(buf));
        if (v) {
            if (v->is_str) v->scalar = make_str(buf);
            else { value_t tmp = make_str(buf); v->scalar.is_str = 0; v->scalar.n = to_num(&tmp); }
        }
        if (!match_op(',')) break;
    }
}

static void exec_grid_stmt(void) {
    /* cursor is on the GRID.ID name; the dotted name is stored in text */
    char name[64];
    size_t k = 0; while (cur()->text[k] && k < sizeof(name)-1) { name[k] = cur()->text[k]; k++; } name[k] = '\0';
    advance();
    if (strequal(name, "GRID.CLS")) { console_clear(); return; }
    if (strequal(name, "GRID.LOG")) { value_t v = eval_expr(); char b[180]; if(v.is_str){size_t j=0;while(v.s[j]&&j<sizeof(b)-1){b[j]=v.s[j];j++;}b[j]='\0';}else num_to_string(v.n,b,sizeof(b)); log_event(b); return; }
    if (strequal(name, "GRID.SPAWN")) { value_t v = eval_expr(); char n[40]; if(v.is_str){size_t j=0;while(v.s[j]&&j<sizeof(n)-1){n[j]=v.s[j];j++;}n[j]='\0';}else num_to_string(v.n,n,sizeof(n)); program_spawn_named(n); return; }
    if (strequal(name, "GRID.SERIAL.WRITE")) { value_t v = eval_expr(); if(v.is_str) serial_write(v.s); else { char b[32]; num_to_string(v.n,b,sizeof(b)); serial_write(b); } return; }
    if (strequal(name, "GRID.COLOR")) { value_t v = eval_expr(); console_set_color((uint8_t)(to_num(&v) / BASIC_SCALE)); return; }
    if (strequal(name, "GRID.WAIT")) { value_t v = eval_expr(); uint32_t ticks = (uint32_t)(to_num(&v) / BASIC_SCALE); uint32_t start = timer_ticks(); while (timer_ticks() - start < ticks && g_running) { /* spin */ } return; }
    if (strequal(name, "GRID.PRINT")) { /* alias with extra newline */ value_t v = eval_expr(); int col=0; print_value(&v,0,&col); console_write_line(""); return; }
    if (strequal(name, "GRID.IRC.CONNECT")) {
        value_t host = eval_expr(); if (!match_op(',')) { set_error("IRC.CONNECT: need ,"); return; }
        value_t portv = eval_expr(); if (!match_op(',')) { set_error("IRC.CONNECT: need ,"); return; }
        value_t nick = eval_expr();
        char hs[64], ns[40];
        if (host.is_str) { size_t j=0; while(host.s[j]&&j<sizeof(hs)-1){hs[j]=host.s[j];j++;} hs[j]='\0'; }
        else { num_to_string(host.n, hs, sizeof(hs)); }
        if (nick.is_str) { size_t j=0; while(nick.s[j]&&j<sizeof(ns)-1){ns[j]=nick.s[j];j++;} ns[j]='\0'; }
        else { num_to_string(nick.n, ns, sizeof(ns)); }
        uint16_t port = (uint16_t)(to_num(&portv) / BASIC_SCALE);
        if (irc_connect(hs, port, ns) != 0) { set_error("IRC: connect failed"); }
        return;
    }
    if (strequal(name, "GRID.IRC.JOIN")) {
        value_t v = eval_expr(); char ch[64];
        if (v.is_str) { size_t j=0; while(v.s[j]&&j<sizeof(ch)-1){ch[j]=v.s[j];j++;} ch[j]='\0'; }
        else { num_to_string(v.n, ch, sizeof(ch)); }
        if (irc_join(ch) != 0) { set_error("IRC: join failed"); }
        return;
    }
    if (strequal(name, "GRID.IRC.PART")) {
        value_t v = eval_expr(); char ch[64];
        if (v.is_str) { size_t j=0; while(v.s[j]&&j<sizeof(ch)-1){ch[j]=v.s[j];j++;} ch[j]='\0'; }
        else { num_to_string(v.n, ch, sizeof(ch)); }
        if (irc_part(ch) != 0) { set_error("IRC: part failed"); }
        return;
    }
    if (strequal(name, "GRID.IRC.SAY") || strequal(name, "GRID.IRC.MSG")) {
        value_t tgt = eval_expr(); if (!match_op(',')) { set_error("IRC.SAY: need ,"); return; }
        value_t msg = eval_expr();
        char tbuf[64], mbuf[256];
        if (tgt.is_str) { size_t j=0; while(tgt.s[j]&&j<sizeof(tbuf)-1){tbuf[j]=tgt.s[j];j++;} tbuf[j]='\0'; }
        else { num_to_string(tgt.n, tbuf, sizeof(tbuf)); }
        if (msg.is_str) { size_t j=0; while(msg.s[j]&&j<sizeof(mbuf)-1){mbuf[j]=msg.s[j];j++;} mbuf[j]='\0'; }
        else { num_to_string(msg.n, mbuf, sizeof(mbuf)); }
        if (irc_say(tbuf, mbuf) != 0) { set_error("IRC: say failed"); }
        return;
    }
    if (strequal(name, "GRID.IRC.NICK")) {
        value_t v = eval_expr(); char nb[40];
        if (v.is_str) { size_t j=0; while(v.s[j]&&j<sizeof(nb)-1){nb[j]=v.s[j];j++;} nb[j]='\0'; }
        else { num_to_string(v.n, nb, sizeof(nb)); }
        if (irc_nick(nb) != 0) { set_error("IRC: nick failed"); }
        return;
    }
    if (strequal(name, "GRID.IRC.QUIT")) { irc_quit("GridBASIC"); return; }
    if (strequal(name, "GRID.IRC.POLL")) { irc_poll(); return; }
    if (strequal(name, "GRID.IRC.DISCONNECT")) { irc_disconnect(); return; }
    if (strequal(name, "GRID.BTC.SEND")) {
        value_t addr = eval_expr(); if (!match_op(',')) { set_error("BTC.SEND: need ,"); return; }
        value_t amt = eval_expr();
        char ab[128], mb[64], rb[BTC_RESP_MAX];
        if (addr.is_str) { size_t j=0; while(addr.s[j]&&j<sizeof(ab)-1){ab[j]=addr.s[j];j++;} ab[j]='\0'; }
        else { num_to_string(addr.n, ab, sizeof(ab)); }
        if (amt.is_str) { size_t j=0; while(amt.s[j]&&j<sizeof(mb)-1){mb[j]=amt.s[j];j++;} mb[j]='\0'; }
        else { num_to_string(amt.n, mb, sizeof(mb)); }
        if (btc_send(ab, mb, rb, sizeof(rb)) != 0) { set_error(rb); }
        return;
    }
    set_error("GRID: unknown statement");
}

static int find_label(int lineno) {
    for (int i = 0; i < g_nlabels; ++i) {
        if (g_labels[i] == lineno) return g_label_tok[i];
    }
    return -1;
}

static void skip_to_newline(void) {
    while (cur()->type != T_NEWLINE && cur()->type != T_EOF) advance();
}

static void skip_to_else_or_newline(void) {
    while (cur()->type != T_NEWLINE && cur()->type != T_EOF) {
        if (cur()->type == T_KW && cur()->kw == KW_ELSE) return;
        advance();
    }
}

/* execute the statement at the cursor; returns control hints via globals */
static void exec_statement(void) {
    token_t *t = cur();
    if (t->type == T_NEWLINE || t->type == T_EOF) return;

    if (t->type == T_KW) {
        switch (t->kw) {
        case KW_PRINT: advance(); exec_print(); return;
        case KW_LET:   advance(); exec_assign(); return;
        case KW_IF:    advance(); {
            value_t cond = eval_expr();
            if (!match_kw(KW_THEN)) { set_error("IF: missing THEN"); return; }
            if (to_bool(&cond)) {
                /* THEN may be a line number or a statement */
                if (cur()->type == T_NUM) {
                    int ln = (int)(cur()->num / BASIC_SCALE); advance();
                    int target = find_label(ln);
                    if (target < 0) { set_error("GOTO: no such line"); return; }
                    g_cur = target; return;
                }
                exec_statement();   /* run inline statement */
                /* after inline statement, if we hit ELSE skip it */
                if (cur()->type == T_KW && cur()->kw == KW_ELSE) skip_to_newline();
                return;
            } else {
                skip_to_else_or_newline();
                if (cur()->type == T_KW && cur()->kw == KW_ELSE) {
                    advance();
                    exec_statement();
                }
                return;
            }
        }
        case KW_FOR: advance(); {
            if (cur()->type != T_ID) { set_error("FOR: need variable"); return; }
            char name[64]; size_t k=0; while(cur()->text[k]&&k<sizeof(name)-1){name[k]=cur()->text[k];k++;} name[k]='\0';
            advance();
            if (!match_op('=')) { set_error("FOR: need ="); return; }
            value_t start = eval_expr();
            if (!match_kw(KW_TO)) { set_error("FOR: need TO"); return; }
            value_t end = eval_expr();
            value_t step; step.is_str=0; step.n = BASIC_SCALE;
            if (match_kw(KW_STEP)) step = eval_expr();
            var_t *v = get_var(name, 0);
            if (!v) return;
            v->scalar.is_str = 0; v->scalar.n = to_num(&start);
            if (g_for_sp >= MAX_FRAMES) { set_error("FOR: too deep"); return; }
            for_frame_t *f = &g_for_stack[g_for_sp++];
            f->var_index = (int)(v - g_vars);
            f->end_val = to_num(&end);
            f->step_val = to_num(&step);
            f->tok_index = g_cur;
            return;
        }
        case KW_NEXT: advance(); {
            if (g_for_sp <= 0) { set_error("NEXT: without FOR"); return; }
            for_frame_t *f = &g_for_stack[g_for_sp - 1];
            var_t *v = &g_vars[f->var_index];
            if (cur()->type == T_ID) { advance(); }
            v->scalar.n += f->step_val;
            int cont = (f->step_val >= 0) ? (v->scalar.n <= f->end_val) : (v->scalar.n >= f->end_val);
            if (cont) {
                g_cur = f->tok_index;
            } else {
                g_for_sp--;
            }
            return;
        }
        case KW_WHILE: advance(); {
            if (g_for_sp >= MAX_FRAMES) { set_error("WHILE: too deep"); return; }
            int cond_start = g_cur;
            for_frame_t *f = &g_for_stack[g_for_sp++];
            f->tok_index = cond_start;
            f->var_index = -1;
            value_t cond = eval_expr();
            if (!to_bool(&cond)) {
                /* skip to matching WEND */
                int depth = 1;
                while (cur()->type != T_EOF) {
                    if (cur()->type == T_KW && cur()->kw == KW_WHILE) depth++;
                    else if (cur()->type == T_KW && cur()->kw == KW_WEND) { depth--; if (depth == 0) { advance(); break; } }
                    advance();
                }
                g_for_sp--;
            }
            /* else: g_cur is past the condition; body runs from here */
            return;
        }
        case KW_WEND: advance(); {
            if (g_for_sp <= 0 || g_for_stack[g_for_sp-1].var_index != -1) { set_error("WEND: without WHILE"); return; }
            for_frame_t *f = &g_for_stack[g_for_sp-1];
            int after_wend = g_cur;
            g_cur = f->tok_index;          /* rewind to condition */
            value_t cond = eval_expr();    /* re-evaluate; g_cur now past condition */
            if (to_bool(&cond)) {
                /* continue body (g_cur already past condition) */
            } else {
                g_for_sp--;                /* pop frame */
                g_cur = after_wend;        /* exit past WEND */
            }
            return;
        }
        case KW_REPEAT: advance(); {
            if (g_for_sp >= MAX_FRAMES) { set_error("REPEAT: too deep"); return; }
            for_frame_t *f = &g_for_stack[g_for_sp++];
            f->tok_index = g_cur; f->var_index = -2;
            return;
        }
        case KW_UNTIL: advance(); {
            if (g_for_sp <= 0 || g_for_stack[g_for_sp-1].var_index != -2) { set_error("UNTIL: without REPEAT"); return; }
            value_t cond = eval_expr();
            if (to_bool(&cond)) g_for_sp--;
            else g_cur = g_for_stack[g_for_sp-1].tok_index;
            return;
        }
        case KW_GOTO: advance(); {
            if (cur()->type != T_NUM) { set_error("GOTO: need line number"); return; }
            int ln = (int)(cur()->num / BASIC_SCALE); advance();
            int target = find_label(ln);
            if (target < 0) { set_error("GOTO: no such line"); return; }
            g_cur = target; return;
        }
        case KW_GOSUB: advance(); {
            if (cur()->type != T_NUM) { set_error("GOSUB: need line number"); return; }
            int ln = (int)(cur()->num / BASIC_SCALE); advance();
            int target = find_label(ln);
            if (target < 0) { set_error("GOSUB: no such line"); return; }
            if (g_gosub_sp >= MAX_FRAMES) { set_error("GOSUB: too deep"); return; }
            g_gosub_stack[g_gosub_sp++].tok_index = g_cur;
            g_cur = target; return;
        }
        case KW_RETURN: advance(); {
            if (g_gosub_sp <= 0) { set_error("RETURN: without GOSUB"); return; }
            g_cur = g_gosub_stack[--g_gosub_sp].tok_index; return;
        }
        case KW_INPUT: advance(); exec_input(); return;
        case KW_DIM:   advance(); exec_dim(); return;
        case KW_REM:   skip_to_newline(); return;
        case KW_END: case KW_STOP: advance(); g_running = 0; return;
        case KW_THEN: case KW_ELSE: case KW_TO: case KW_STEP:
        case KW_AND: case KW_OR: case KW_NOT: case KW_MOD: case KW_DIV:
            set_error("SYNTAX: unexpected keyword"); return;
        case KW_NONE: break;
        }
    }

    if (t->type == T_ID) {
        if (str_contains(t->text, '.')) {
            exec_grid_stmt();
            return;
        }
        exec_assign();
        return;
    }
    if (t->type == T_OP && t->op == '?') {   /* ? is PRINT shorthand */
        advance();
        exec_print();
        return;
    }
    set_error("SYNTAX: unexpected token");
}

static void run_loop(void) {
    while (g_running && !g_error) {
        if (g_cur >= g_ntok) break;
        token_t *t = cur();
        if (t->type == T_EOF) break;
        if (t->type == T_NEWLINE) { g_cur++; continue; }
        if (t->type == T_OP && t->op == ':') { g_cur++; continue; }  /* statement separator */
        exec_statement();
        if (g_error) break;
    }
}

/* ---- runtime math/string helpers (forward-declared above) ---- */

static num_t sqrt_local(num_t x) {
    /* x is value*SCALE; compute sqrt(x) in fixed-point = sqrt(value)*SCALE.
     * sqrt(x_fp) where x_fp = v*SCALE: sqrt(v*SCALE) = sqrt(v)*sqrt(SCALE).
     * We want result = sqrt(v)*SCALE, so result = sqrt(v*SCALE) * SCALE / sqrt(SCALE)
     *   = sqrt(x_fp) * sqrt(SCALE). Use integer Newton on __int128. */
    if (x <= 0) return 0;
    /* target: r such that r*r == x * SCALE (since (sqrt(v)*SCALE)^2 = v*SCALE^2 = x*SCALE) */
    __int128 target = (__int128)x * BASIC_SCALE;
    __int128 r = target;
    /* initial estimate */
    if (r > (__int128)1 << 60) r = (__int128)1 << 60;
    for (int i = 0; i < 80; ++i) {
        if (r == 0) r = 1;
        __int128 next = (r + target / r) / 2;
        if (next >= r && next - r < 2) { r = next; break; }
        if (r >= next && r - next < 2) { r = next; break; }
        r = next;
    }
    return (num_t)r;
}

static num_t pow_local(num_t base, num_t exp) {
    /* integer exponent only (exp is fixed-point; use exp/SCALE as integer) */
    int e = (int)(exp / BASIC_SCALE);
    if (e == 0) return BASIC_SCALE;
    int n = e < 0 ? -e : e;
    int neg = e < 0;
    __int128 r = BASIC_SCALE;
    __int128 b = base;
    for (int i = 0; i < n; ++i) {
        r = r * b / BASIC_SCALE;
    }
    if (neg) r = (__int128)BASIC_SCALE * BASIC_SCALE / r;
    return (num_t)r;
}

static uint32_t g_rnd_state = 0xC0FFEEu;
static uint32_t rnd_local(void) {
    g_rnd_state = g_rnd_state * 1664525u + 1013904223u;
    return g_rnd_state;
}

static int str_len(const char *s) { int n = 0; while (s[n]) n++; return n; }
static int str_cmp(const char *a, const char *b) {
    while (*a && *b) { if (*a != *b) return *a < *b ? -1 : 1; a++; b++; }
    if (*a == *b) return 0;
    return *a ? 1 : -1;
}

static int str_contains(const char *s, char ch) {
    for (size_t i = 0; s[i]; ++i) if (s[i] == ch) return 1;
    return 0;
}

/* ---- public API ---- */

static char *append_char(char *p, char *end, char c) {
    if (p < end) {
        *p++ = c;
    }
    return p;
}

static void join_source(const char *src, char *out, size_t cap) {
    /* ensure program ends with a newline */
    size_t k = 0;
    while (src[k] && k < cap - 2) { out[k] = src[k]; k++; }
    if (k > 0 && out[k-1] != '\n') out[k++] = '\n';
    out[k] = '\0';
}

int basic_run_source(const char *source) {
    static char src_buf[8192];
    join_source(source, src_buf, sizeof(src_buf));
    if (tokenize(src_buf) != 0) {
        console_set_color(GRID_COL_ERROR);
        console_write_line("GridBASIC: token limit exceeded (program too large)");
        console_set_color(GRID_COL_DEFAULT);
        return -1;
    }
    reset_state();
    run_loop();
    if (g_error) {
        console_set_color(GRID_COL_ERROR);
        console_write("GridBASIC error: ");
        console_write_line(g_errmsg);
        int ln = (g_cur < g_ntok) ? g_tokens[g_cur].line_no + 1 : -1;
        char b[16]; num_to_string((num_t)ln * BASIC_SCALE, b, sizeof(b));
        console_write("  at line ");
        console_write_line(b);
        console_set_color(GRID_COL_DEFAULT);
        return -1;
    }
    return 0;
}

int basic_run_file(const char *path) {
    static char buf[8192];
    static char pathbuf[128];
    size_t got = 0;
    size_t k = 0;
    if (!path) {
        return -1;
    }
    while (path[k] && k + 1 < sizeof(pathbuf)) {
        char c = path[k];
        if (c == '\r' || c == '\n') {
            break;
        }
        pathbuf[k] = c;
        k++;
    }
    pathbuf[k] = '\0';
    while (k > 0 && (pathbuf[k - 1] == ' ' || pathbuf[k - 1] == '\t')) {
        pathbuf[--k] = '\0';
    }
    if (gfs_read_file(pathbuf, buf, sizeof(buf) - 1, &got) != 0) {
        console_set_color(GRID_COL_ERROR);
        console_write("GridBASIC: cannot read ");
        console_write_line(pathbuf);
        console_set_color(GRID_COL_DEFAULT);
        return -1;
    }
    buf[got] = '\0';
    return basic_run_source(buf);
}

void basic_print_version(void) {
    console_set_color(GRID_COL_TITLE);
    console_write_line("GridBASIC 6.2 — Advanced BASIC for the Grid");
    console_set_color(GRID_COL_DEFAULT);
    console_write_line("PRINT LET IF/THEN/ELSE FOR/TO/STEP/NEXT WHILE/WEND REPEAT/UNTIL");
    console_write_line("GOTO GOSUB/RETURN INPUT DIM REM END  +  GRID.* / GRID.AI.* / GRID.IRC.* / GRID.BTC.* bindings");
}

/* keep append_char referenced */
void basic_internal_noop(void) { char b[4]; char *p = b; p = append_char(p, b+4, 'x'); (void)p; }
