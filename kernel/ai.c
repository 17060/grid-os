#include "ai.h"

#include "net.h"
#include "serial.h"
#include "tcp.h"

#include <stddef.h>
#include <stdint.h>

#define AI_HOST_IP   0x0A000202u  /* 10.0.2.2 */
#define AI_TCP_PORT  8766u
#define AI_HDR       "GRIDAI/1.0/"
#define AI_OK        "GRIDAI/1.0/OK"
#define AI_ERR       "GRIDAI/1.0/ERR"
#define AI_END       "#GRIDAI/END"

typedef enum {
    AI_ACT_ASK,
    AI_ACT_EXPLAIN,
    AI_ACT_FIX,
    AI_ACT_COMPLETE,
    AI_ACT_MODELS
} ai_action_t;

static size_t ai_strlen(const char *s) {
    size_t n = 0;
    if (!s) {
        return 0;
    }
    while (s[n]) {
        n++;
    }
    return n;
}

static void ai_strcpy(char *d, size_t cap, const char *s) {
    size_t n = 0;
    if (!d || cap == 0) {
        return;
    }
    if (!s) {
        d[0] = '\0';
        return;
    }
    while (s[n] && n + 1 < cap) {
        d[n] = s[n];
        n++;
    }
    d[n] = '\0';
}

static int ai_streq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

static char ai_upper(char c) {
    if (c >= 'a' && c <= 'z') {
        return (char)(c - 32);
    }
    return c;
}

static int ai_contains(const char *hay, const char *needle) {
    size_t nlen = ai_strlen(needle);
    if (nlen == 0) {
        return 0;
    }
    for (size_t i = 0; hay[i]; ++i) {
        size_t j = 0;
        while (j < nlen && hay[i + j] == needle[j]) {
            j++;
        }
        if (j == nlen) {
            return 1;
        }
    }
    return 0;
}

static int ai_has_word(const char *hay, const char *needle) {
    size_t nlen = ai_strlen(needle);
    if (nlen == 0) {
        return 0;
    }
    for (size_t i = 0; hay[i]; ++i) {
        size_t j = 0;
        while (j < nlen && hay[i + j] && ai_upper(hay[i + j]) == needle[j]) {
            j++;
        }
        if (j == nlen) {
            char next = hay[i + nlen];
            if (next == '\0' || next == ' ' || next == '(' || next == '.') {
                return 1;
            }
        }
    }
    return 0;
}

static const char *action_tag(ai_action_t act) {
    switch (act) {
    case AI_ACT_ASK:      return "ASK";
    case AI_ACT_EXPLAIN:  return "EXPLAIN";
    case AI_ACT_FIX:      return "FIX";
    case AI_ACT_COMPLETE: return "COMPLETE";
    case AI_ACT_MODELS:   return "MODELS";
    }
    return "ASK";
}

static size_t build_request(ai_action_t act, const char *input, char *buf, size_t cap) {
    const char *tag = action_tag(act);
    size_t p = 0;
    const char *hdr = AI_HDR;
    while (*hdr && p + 1 < cap) {
        buf[p++] = *hdr++;
    }
    for (size_t i = 0; tag[i] && p + 1 < cap; ++i) {
        buf[p++] = tag[i];
    }
    if (p + 1 < cap) {
        buf[p++] = '\n';
    }
    if (input) {
        for (size_t i = 0; input[i] && p + 1 < cap; ++i) {
            char c = input[i];
            if (c == '\r') {
                continue;
            }
            buf[p++] = c;
        }
    }
    if (p + 1 < cap) {
        buf[p++] = '\n';
    }
    const char *end = AI_END;
    while (*end && p + 1 < cap) {
        buf[p++] = *end++;
    }
    if (p + 1 < cap) {
        buf[p++] = '\n';
    }
    buf[p] = '\0';
    return p;
}

static int parse_response(const char *raw, char *out, size_t cap) {
    const char *p = raw;
    int in_body = 0;

    out[0] = '\0';
    while (*p) {
        const char *line_start = p;
        while (*p && *p != '\n') {
            p++;
        }
        char line[180];
        size_t len = (size_t)(p - line_start);
        if (len >= sizeof(line)) {
            len = sizeof(line) - 1;
        }
        for (size_t i = 0; i < len; ++i) {
            line[i] = line_start[i];
        }
        line[len] = '\0';
        if (*p == '\n') {
            p++;
        }

        if (ai_streq(line, AI_END)) {
            return in_body ? 0 : -1;
        }
        if (ai_streq(line, AI_OK)) {
            in_body = 1;
            continue;
        }
        if (ai_streq(line, AI_ERR)) {
            in_body = 2;
            continue;
        }
        if (in_body == 1) {
            if (out[0]) {
                size_t ol = ai_strlen(out);
                if (ol + 2 < cap) {
                    out[ol] = '\n';
                    out[ol + 1] = '\0';
                }
            }
            size_t ol = ai_strlen(out);
            for (size_t i = 0; line[i] && ol + 1 < cap; ++i) {
                out[ol++] = line[i];
            }
            out[ol] = '\0';
        } else if (in_body == 2) {
            ai_strcpy(out, cap, line);
            return -1;
        }
    }
    return in_body ? 0 : -1;
}

static int try_tcp_bridge(ai_action_t act, const char *input, char *out, size_t cap) {
    char req[640];
    size_t req_len;
    tcp_conn_t conn;
    char accum[2048];
    size_t acc = 0;
    int got_end = 0;

    if (!net_present()) {
        return -1;
    }

    req_len = build_request(act, input, req, sizeof(req));
    if (tcp_connect(&conn, AI_HOST_IP, (uint16_t)AI_TCP_PORT) != 0) {
        return -1;
    }

    if (tcp_send(&conn, req, req_len) != 0) {
        tcp_close(&conn);
        return -1;
    }

    accum[0] = '\0';
    for (int round = 0; round < 300 && !got_end; ++round) {
        int n = tcp_recv(&conn, 400);
        if (n > 0 && conn.rx_len > 0) {
            for (size_t i = 0; i < conn.rx_len && acc + 1 < sizeof(accum); ++i) {
                accum[acc++] = (char)conn.rx_buf[i];
            }
            accum[acc] = '\0';
            conn.rx_len = 0;
            if (ai_contains(accum, AI_END)) {
                got_end = 1;
            }
        }
        if (conn.closed || conn.error) {
            break;
        }
    }
    tcp_close(&conn);

    if (!got_end) {
        return -1;
    }
    return parse_response(accum, out, cap);
}

static int try_serial_bridge(ai_action_t act, const char *input, char *out, size_t cap) {
    char req[640];
    char line[180];
    char body[1024];
    size_t body_len = 0;
    int mode = 0; /* 0=hdr 1=ok 2=err 3=body */

    if (!serial_is_online()) {
        return -1;
    }

    build_request(act, input, req, sizeof(req));
    serial_write(req);

    body[0] = '\0';
    for (int attempt = 0; attempt < 512; ++attempt) {
        size_t len = serial_read_line(line, sizeof(line), 800000);
        if (len == 0) {
            continue;
        }
        if (ai_streq(line, AI_END)) {
            ai_strcpy(out, cap, body);
            return mode == 1 ? 0 : -1;
        }
        if (ai_streq(line, AI_OK)) {
            mode = 1;
            body_len = 0;
            body[0] = '\0';
            continue;
        }
        if (ai_streq(line, AI_ERR)) {
            mode = 2;
            body_len = 0;
            body[0] = '\0';
            continue;
        }
        if (mode == 1 || mode == 2) {
            if (body_len > 0 && body_len + 1 < sizeof(body)) {
                body[body_len++] = '\n';
                body[body_len] = '\0';
            }
            for (size_t i = 0; line[i] && body_len + 1 < sizeof(body); ++i) {
                body[body_len++] = line[i];
            }
            body[body_len] = '\0';
        }
    }
    return -1;
}

static void offline_explain(const char *line, char *out, size_t cap) {
    if (!line || !line[0]) {
        ai_strcpy(out, cap, "EXPLAIN: place cursor on a BASIC line or pass text.");
        return;
    }
    if (ai_has_word(line, "PRINT") || ai_has_word(line, "?")) {
        ai_strcpy(out, cap, "PRINT outputs values. Use ; to suppress newline, , for tab columns.");
        return;
    }
    if (ai_has_word(line, "FOR")) {
        ai_strcpy(out, cap, "FOR i = start TO end [STEP n] ... NEXT i runs a counted loop.");
        return;
    }
    if (ai_has_word(line, "IF")) {
        ai_strcpy(out, cap, "IF condition THEN statement [ELSE statement]. THEN may be inline or a line number.");
        return;
    }
    if (ai_has_word(line, "WHILE")) {
        ai_strcpy(out, cap, "WHILE condition ... WEND repeats while condition is true.");
        return;
    }
    if (ai_has_word(line, "GOTO")) {
        ai_strcpy(out, cap, "GOTO line_number jumps to a numbered line label.");
        return;
    }
    if (ai_has_word(line, "GOSUB")) {
        ai_strcpy(out, cap, "GOSUB line_number calls a subroutine; use RETURN to resume.");
        return;
    }
    if (ai_has_word(line, "DIM")) {
        ai_strcpy(out, cap, "DIM A(n) declares a 0..n array. String arrays use A$(n).");
        return;
    }
    if (ai_has_word(line, "INPUT")) {
        ai_strcpy(out, cap, "INPUT [\"prompt\";] var reads a line from the console into var.");
        return;
    }
    if (ai_has_word(line, "GRID.AI")) {
        ai_strcpy(out, cap, "GRID.AI.* calls AI helpers: ASK$, EXPLAIN$, FIX$, COMPLETE$, MODELS$.");
        return;
    }
    if (ai_has_word(line, "GRID.")) {
        ai_strcpy(out, cap, "GRID.* bindings reach the OS: CLS LOG SPAWN SERIAL TIME RND PING STATUS$ and AI.*.");
        return;
    }
    ai_strcpy(out, cap, "Offline help: try PRINT FOR IF WHILE GOTO GOSUB DIM INPUT or GRID.* bindings.");
}

static void offline_fix(const char *code, char *out, size_t cap) {
    if (!code || !code[0]) {
        ai_strcpy(out, cap, "FIX: pass GridBASIC source in code$.");
        return;
    }
    if (!ai_has_word(code, "END") && !ai_has_word(code, "STOP")) {
        ai_strcpy(out, cap, "Suggestion: add END on the last line to terminate the program.");
        return;
    }
    if (ai_has_word(code, "FOR") && !ai_has_word(code, "NEXT")) {
        ai_strcpy(out, cap, "Suggestion: every FOR needs a matching NEXT variable.");
        return;
    }
    if (ai_has_word(code, "WHILE") && !ai_has_word(code, "WEND")) {
        ai_strcpy(out, cap, "Suggestion: every WHILE needs WEND.");
        return;
    }
    if (ai_has_word(code, "GOSUB") && !ai_has_word(code, "RETURN")) {
        ai_strcpy(out, cap, "Suggestion: GOSUB subroutines should RETURN.");
        return;
    }
    ai_strcpy(out, cap, "No obvious issue found (offline). Run make ai-bridge for LLM fixes.");
}

static void offline_complete(const char *code, char *out, size_t cap) {
    if (!code || !code[0]) {
        ai_strcpy(out, cap, "10 PRINT \"hello grid\"\n20 END");
        return;
    }
    if (ai_has_word(code, "FOR") && !ai_has_word(code, "NEXT")) {
        ai_strcpy(out, cap, "  PRINT I\nNEXT I");
        return;
    }
    if (ai_has_word(code, "IF") && !ai_has_word(code, "THEN")) {
        ai_strcpy(out, cap, " THEN PRINT \"ok\"");
        return;
    }
    ai_strcpy(out, cap, "\nPRINT \"done\"\nEND");
}

static void offline_models(char *out, size_t cap) {
    ai_strcpy(out, cap, "offline-builtin (run: make ai-bridge on host for LLM)");
}

static void offline_ask(const char *prompt, char *out, size_t cap) {
    (void)prompt;
    ai_strcpy(out, cap,
              "Grid AI offline. Keywords: PRINT FOR IF WHILE GRID.*. "
              "Host: make ai-bridge then GRID.AI.ASK$ or :ai ask ...");
}

static int ai_dispatch(ai_action_t act, const char *input, char *out, size_t cap) {
    if (try_tcp_bridge(act, input, out, cap) == 0) {
        return 0;
    }
    if (try_serial_bridge(act, input, out, cap) == 0) {
        return 0;
    }
    switch (act) {
    case AI_ACT_ASK:
        offline_ask(input, out, cap);
        break;
    case AI_ACT_EXPLAIN:
        offline_explain(input, out, cap);
        break;
    case AI_ACT_FIX:
        offline_fix(input, out, cap);
        break;
    case AI_ACT_COMPLETE:
        offline_complete(input, out, cap);
        break;
    case AI_ACT_MODELS:
        offline_models(out, cap);
        break;
    }
    return 0;
}

int ai_ask(const char *prompt, char *out, size_t cap) {
    return ai_dispatch(AI_ACT_ASK, prompt ? prompt : "", out, cap);
}

int ai_explain(const char *line, char *out, size_t cap) {
    return ai_dispatch(AI_ACT_EXPLAIN, line ? line : "", out, cap);
}

int ai_fix(const char *code, char *out, size_t cap) {
    return ai_dispatch(AI_ACT_FIX, code ? code : "", out, cap);
}

int ai_complete(const char *code, char *out, size_t cap) {
    return ai_dispatch(AI_ACT_COMPLETE, code ? code : "", out, cap);
}

int ai_models(char *out, size_t cap) {
    return ai_dispatch(AI_ACT_MODELS, "", out, cap);
}
