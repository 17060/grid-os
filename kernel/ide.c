#include "console.h"
#include "gfs.h"
#include "ide.h"
#include "log.h"
#include "mouse.h"
#include "program.h"
#include "security.h"

#include <stddef.h>
#include <stdint.h>

#define IDE_SOURCE_MAX 4096
#define IDE_LINE_MAX   80
#define IDE_LINES_MAX  64
#define IDE_PATH_MAX   GFS_PATH_MAX
#define IDE_FILESEL_MAX 16

#define ROW_MENU    0
#define ROW_TITLE   1
#define ROW_BODY    2
#define ROW_BODY_MAX 16
#define ROW_BOTTOM  18
#define ROW_STATUS  19
#define ROW_AMIGA   21
#define ROW_PROMPT  22
#define PROMPT_COL  18

#define VGA_MEMORY ((volatile uint16_t *)0xB8000)

static int mouse_cursor_x = -1;
static int mouse_cursor_y = -1;
static uint16_t mouse_cursor_saved = 0;

typedef enum {
    IDE_VIEW_EDITOR = 0,
    IDE_VIEW_DESKTOP = 1,
    IDE_VIEW_FILESEL = 2,
    IDE_VIEW_CLI = 3
} ide_view_t;

typedef struct {
    int number;
    char text[IDE_LINE_MAX];
} script_line_t;

static char source[IDE_SOURCE_MAX];
static char current_path[IDE_PATH_MAX] = "/source/untitled.grid";
static char assign_name[16] = "source:";
static char assign_path[32] = "/source";
static int modified = 0;
static ide_view_t current_view = IDE_VIEW_EDITOR;
static int filesel_save = 0;
static char filesel_paths[IDE_FILESEL_MAX][GFS_PATH_MAX];
static int filesel_count = 0;
static int script_out_row = 0;

static int char_lower(int c) {
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}

static int equals_ci(const char *a, const char *b) {
    while (*a && *b) {
        if (char_lower((unsigned char)*a) != char_lower((unsigned char)*b)) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

static int starts_with_ci(const char *text, const char *prefix) {
    while (*prefix) {
        if (char_lower((unsigned char)*text++) != char_lower((unsigned char)*prefix++)) {
            return 0;
        }
    }
    return 1;
}

static void copy_string(char *dest, size_t dest_len, const char *src) {
    size_t i = 0;
    if (src) {
        while (src[i] && i + 1 < dest_len) {
            dest[i] = src[i];
            i++;
        }
    }
    dest[i] = '\0';
}

static void skip_spaces(const char **cursor) {
    while (**cursor == ' ') {
        (*cursor)++;
    }
}

static size_t strlen_simple(const char *s) {
    size_t n = 0;
    while (s && s[n]) {
        n++;
    }
    return n;
}

static int parse_line_number(const char **cursor, int *out_num) {
    int value = 0;
    int digits = 0;

    skip_spaces(cursor);
    while (**cursor >= '0' && **cursor <= '9') {
        value = value * 10 + (**cursor - '0');
        (*cursor)++;
        digits++;
    }
    if (digits == 0) {
        return 0;
    }
    *out_num = value;
    return 1;
}

static const char * basename_from_path(const char *path) {
    const char *base = path;
    for (const char *p = path; *p; ++p) {
        if (*p == '/') {
            base = p + 1;
        }
    }
    return base;
}

static void set_assign(const char *name, const char *path) {
    copy_string(assign_name, sizeof(assign_name), name);
    copy_string(assign_path, sizeof(assign_path), path);
}

static int resolve_volume(const char *token, const char **rest_out, char *path_out, size_t path_len) {
    const char *rest = token;
    skip_spaces(&rest);

    if (starts_with_ci(rest, "source:")) {
        copy_string(path_out, path_len, "/source/");
        rest += 7;
    } else if (starts_with_ci(rest, "programs:")) {
        copy_string(path_out, path_len, "/programs/");
        rest += 9;
    } else if (starts_with_ci(rest, "flynn:")) {
        copy_string(path_out, path_len, "/flynn/");
        rest += 6;
    } else if (starts_with_ci(rest, "grid:")) {
        copy_string(path_out, path_len, "/grid/");
        rest += 5;
    } else if (starts_with_ci(rest, "sys:")) {
        copy_string(path_out, path_len, "/");
        rest += 4;
    } else if (rest[0] == '/') {
        size_t i = 0;
        while (*rest && *rest != ' ' && i + 1 < path_len) {
            path_out[i++] = *rest++;
        }
        path_out[i] = '\0';
        if (rest_out) {
            *rest_out = rest;
        }
        return 0;
    } else {
        copy_string(path_out, path_len, assign_path);
        size_t len = strlen_simple(path_out);
        if (len > 0 && path_out[len - 1] != '/' && len + 1 < path_len) {
            path_out[len] = '/';
            path_out[len + 1] = '\0';
        }
    }

    if (rest_out) {
        *rest_out = rest;
    }
    return 0;
}

static void build_full_path(const char *token, char *out, size_t out_len) {
    char base[IDE_PATH_MAX];
    const char *rest = 0;

    resolve_volume(token, &rest, base, sizeof(base));
    if (!rest || *rest == '\0') {
        copy_string(out, out_len, base);
        return;
    }

    skip_spaces(&rest);
    size_t pos = 0;
    for (size_t i = 0; base[i] && pos + 1 < out_len; ++i) {
        out[pos++] = base[i];
    }
    for (size_t i = 0; rest[i] && rest[i] != ' ' && pos + 1 < out_len; ++i) {
        out[pos++] = rest[i];
    }
    out[pos] = '\0';
}

static void ide_clear_source(void) {
    source[0] = '\0';
    modified = 0;
}

static void ide_new_file(void) {
    copy_string(current_path, sizeof(current_path), "/source/untitled.grid");
    ide_clear_source();
}

static int ide_load(const char *path) {
    size_t len = 0;

    if (!path || !gfs_present()) {
        return -1;
    }

    if (gfs_read_file(path, source, sizeof(source) - 1, &len) != 0) {
        return -1;
    }

    source[len] = '\0';
    copy_string(current_path, sizeof(current_path), path);
    modified = 0;
    current_view = IDE_VIEW_EDITOR;
    return 0;
}

static int ide_save(void) {
    size_t len = 0;

    if (!gfs_present()) {
        return -1;
    }
    if (!security_require_capability(CAP_STORAGE, "workshop save")) {
        return -1;
    }

    while (source[len]) {
        len++;
    }

    if (gfs_write_file(current_path, source, len) != 0) {
        return -1;
    }

    modified = 0;
    log_event("workbench saved");
    return 0;
}

static int rebuild_source(const script_line_t *lines, int count) {
    size_t pos = 0;

    for (int i = 0; i < count; ++i) {
        if (lines[i].number <= 0) {
            continue;
        }

        int num = lines[i].number;
        char numbuf[16];
        size_t nlen = 0;
        if (num == 0) {
            numbuf[nlen++] = '0';
        } else {
            char tmp[16];
            size_t tlen = 0;
            while (num > 0) {
                tmp[tlen++] = (char)('0' + (num % 10));
                num /= 10;
            }
            while (tlen > 0) {
                numbuf[nlen++] = tmp[--tlen];
            }
        }
        numbuf[nlen] = '\0';

        for (size_t j = 0; numbuf[j] && pos + 1 < sizeof(source); ++j) {
            source[pos++] = numbuf[j];
        }
        if (pos + 1 >= sizeof(source)) {
            return -1;
        }
        source[pos++] = ' ';

        for (size_t j = 0; lines[i].text[j] && pos + 1 < sizeof(source); ++j) {
            source[pos++] = lines[i].text[j];
        }
        if (pos + 1 >= sizeof(source)) {
            return -1;
        }
        source[pos++] = '\n';
    }

    source[pos] = '\0';
    modified = 1;
    return 0;
}

static int parse_source_lines(script_line_t *lines, int max_lines) {
    int count = 0;
    const char *cursor = source;
    int auto_num = 10;

    while (*cursor && count < max_lines) {
        const char *line_start = cursor;
        while (*cursor && *cursor != '\n') {
            cursor++;
        }

        const char *parse = line_start;
        skip_spaces(&parse);

        if (*parse == '\0') {
            if (*cursor == '\n') {
                cursor++;
            }
            continue;
        }

        int num = 0;
        if (parse_line_number(&parse, &num)) {
            skip_spaces(&parse);
        } else {
            num = auto_num;
        }

        size_t tlen = 0;
        while (*parse && *parse != '\n' && tlen + 1 < IDE_LINE_MAX) {
            lines[count].text[tlen++] = *parse++;
        }
        lines[count].text[tlen] = '\0';

        lines[count].number = num;
        count++;
        auto_num += 10;

        if (*cursor == '\n') {
            cursor++;
        }
    }

    return count;
}

static void sort_lines(script_line_t *lines, int count) {
    for (int i = 0; i < count - 1; ++i) {
        for (int j = i + 1; j < count; ++j) {
            if (lines[j].number < lines[i].number) {
                script_line_t tmp = lines[i];
                lines[i] = lines[j];
                lines[j] = tmp;
            }
        }
    }
}

static void refresh_chrome(void);
static void clear_body(void);

static void script_print_at(const char *text) {
    char line[80];
    size_t i = 0;

    if (script_out_row >= (int)ROW_BOTTOM) {
        return;
    }

    while (*text && i + 1 < sizeof(line)) {
        line[i++] = *text++;
    }
    line[i] = '\0';
    console_write_at(2, (size_t)script_out_row, line, GRID_COL_AMIGA);
    script_out_row++;
}

static void execute_statement(const char *statement) {
    const char *cursor = statement;
    skip_spaces(&cursor);

    if (*cursor == '\0' || *cursor == '\'') {
        return;
    }
    if (starts_with_ci(cursor, "REM")) {
        return;
    }
    if (starts_with_ci(cursor, "CLS")) {
        clear_body();
        script_out_row = (int)ROW_BODY + 1;
        return;
    }
    if (starts_with_ci(cursor, "DISC")) {
        char disc[GRID_DISC_HEX];
        security_format_disc(disc, sizeof(disc));
        script_print_at(disc);
        return;
    }
    if (starts_with_ci(cursor, "CYCLES")) {
        char buf[16];
        uint32_t cycles = security_cycles();
        size_t pos = 0;
        if (cycles == 0) {
            buf[pos++] = '0';
        } else {
            char tmp[16];
            size_t tlen = 0;
            while (cycles > 0) {
                tmp[tlen++] = (char)('0' + (cycles % 10));
                cycles /= 10;
            }
            while (tlen > 0) {
                buf[pos++] = tmp[--tlen];
            }
        }
        buf[pos] = '\0';
        script_print_at(buf);
        return;
    }
    if (starts_with_ci(cursor, "PRINT")) {
        char line[80];
        size_t i = 0;

        cursor += 5;
        skip_spaces(&cursor);
        if (*cursor == '"') {
            cursor++;
            while (*cursor && *cursor != '"' && i + 1 < sizeof(line)) {
                line[i++] = *cursor++;
            }
            line[i] = '\0';
            script_print_at(line);
        } else {
            script_print_at(cursor);
        }
        return;
    }
    if (starts_with_ci(cursor, "SPAWN")) {
        cursor += 5;
        skip_spaces(&cursor);
        if (!security_require_capability(CAP_SPAWN, "gridscript spawn")) {
            return;
        }
        char name[PROGRAM_NAME_MAX];
        size_t i = 0;
        while (*cursor && *cursor != ' ' && i + 1 < sizeof(name)) {
            name[i++] = *cursor++;
        }
        name[i] = '\0';
        if (i > 0) {
            program_spawn_named(name);
        }
        return;
    }
}

static void gridscript_run(void) {
    script_line_t lines[IDE_LINES_MAX];
    int count = parse_source_lines(lines, IDE_LINES_MAX);

    sort_lines(lines, count);

    current_view = IDE_VIEW_CLI;
    clear_body();
    script_out_row = (int)ROW_BODY + 1;
    console_write_at(2, ROW_BODY, "Executing GridScript...", GRID_COL_AMIGA);

    for (int i = 0; i < count; ++i) {
        const char *cursor = lines[i].text;
        skip_spaces(&cursor);
        if (starts_with_ci(cursor, "END") || starts_with_ci(cursor, "STOP")) {
            break;
        }
        if (starts_with_ci(cursor, "REM") || *cursor == '\'') {
            continue;
        }
        execute_statement(lines[i].text);
    }

    script_print_at("Ready.");
    refresh_chrome();
}

static void ide_set_line(int number, const char *text) {
    script_line_t lines[IDE_LINES_MAX];
    int count = parse_source_lines(lines, IDE_LINES_MAX);
    int found = 0;

    for (int i = 0; i < count; ++i) {
        if (lines[i].number == number) {
            copy_string(lines[i].text, sizeof(lines[i].text), text);
            found = 1;
            break;
        }
    }

    if (!found && count < IDE_LINES_MAX) {
        lines[count].number = number;
        copy_string(lines[count].text, sizeof(lines[count].text), text);
        count++;
    }

    sort_lines(lines, count);
    rebuild_source(lines, count);
}

static void ide_delete_line(int number) {
    script_line_t lines[IDE_LINES_MAX];
    int count = parse_source_lines(lines, IDE_LINES_MAX);
    script_line_t kept[IDE_LINES_MAX];
    int kept_count = 0;

    for (int i = 0; i < count; ++i) {
        if (lines[i].number != number) {
            kept[kept_count++] = lines[i];
        }
    }

    rebuild_source(kept, kept_count);
}

static void draw_gem_menu(void) {
    console_fill_row(ROW_MENU, ' ', GRID_COL_TITLE);
    console_write_at(1, ROW_MENU, " Desk ", GRID_COL_TITLE);
    console_write_at(9, ROW_MENU, " File ", GRID_COL_TITLE);
    console_write_at(17, ROW_MENU, " View ", GRID_COL_TITLE);
    console_write_at(25, ROW_MENU, " Options ", GRID_COL_TITLE);
    console_write_at(50, ROW_MENU, "Grid Workbench 1.3 // GEM", GRID_COL_TITLE);
}

static void draw_window_title(const char *title) {
    char bar[82];
    size_t pos = 0;

    bar[pos++] = ' ';
    bar[pos++] = '[';
    bar[pos++] = '#';
    bar[pos++] = ']';
    bar[pos++] = ' ';
    while (*title && pos + 3 < 78) {
        bar[pos++] = *title++;
    }
    while (pos < 77) {
        bar[pos++] = ' ';
    }
    bar[pos++] = ' ';
    bar[pos++] = '[';
    bar[pos++] = '~';
    bar[pos++] = ']';
    bar[pos] = '\0';

    console_fill_row(ROW_TITLE, ' ', GRID_COL_MENU);
    console_write_at(0, ROW_TITLE, bar, GRID_COL_MENU);
}

static void draw_window_bottom(void) {
    console_fill_row(ROW_BOTTOM, ' ', GRID_COL_MENU);
    console_write_at(0, ROW_BOTTOM, " +", GRID_COL_MENU);
    for (size_t x = 2; x < 78; ++x) {
        console_write_at(x, ROW_BOTTOM, "-", GRID_COL_MENU);
    }
    console_write_at(78, ROW_BOTTOM, "+", GRID_COL_MENU);
}

static void draw_gem_status(void) {
    char status[81];
    size_t pos = 0;
    const char *base = basename_from_path(current_path);

    status[pos++] = ' ';
    status[pos++] = '1';
    status[pos++] = ' ';
    status[pos++] = 'B';
    status[pos++] = 'l';
    status[pos++] = 'o';
    status[pos++] = 'c';
    status[pos++] = 'k';
    status[pos++] = ' ';
    status[pos++] = ' ';
    status[pos++] = '|';
    status[pos++] = ' ';
    while (*base && pos + 20 < sizeof(status)) {
        status[pos++] = *base++;
    }
    if (modified) {
        status[pos++] = '*';
    }
    while (pos < 40) {
        status[pos++] = ' ';
    }
    status[pos++] = '|';
    status[pos++] = ' ';
    status[pos++] = 'G';
    status[pos++] = 'F';
    status[pos++] = 'S';
    status[pos++] = ':';
    status[pos++] = gfs_present() ? 'O' : 'X';
    status[pos++] = 'K';
    status[pos] = '\0';

    console_fill_row(ROW_STATUS, ' ', GRID_COL_MENU);
    console_write_at(0, ROW_STATUS, status, GRID_COL_MENU);
}

static void draw_amiga_header(void) {
    console_fill_row(ROW_AMIGA, ' ', GRID_COL_GEM);
    console_write_at(0, ROW_AMIGA, " AmigaDOS // GridOS 6.5.1  Kickstart ready.", GRID_COL_GEM);
}

static void draw_amiga_prompt(void) {
    char prompt[32];
    size_t pos = 0;

    prompt[pos++] = '1';
    prompt[pos++] = '.';
    prompt[pos++] = 'G';
    prompt[pos++] = 'r';
    prompt[pos++] = 'i';
    prompt[pos++] = 'd';
    prompt[pos++] = 'O';
    prompt[pos++] = 'S';
    prompt[pos++] = ':';

    for (size_t i = 0; assign_name[i] && pos + 2 < sizeof(prompt); ++i) {
        prompt[pos++] = assign_name[i];
    }
    prompt[pos++] = '>';
    prompt[pos] = '\0';

    console_fill_row(ROW_PROMPT, ' ', GRID_COL_GEM);
    console_write_at(0, ROW_PROMPT, prompt, GRID_COL_AMIGA);
}

static void refresh_chrome(void) {
    switch (current_view) {
    case IDE_VIEW_DESKTOP:
        draw_window_title("Grid Workbench Desktop");
        break;
    case IDE_VIEW_FILESEL:
        draw_window_title(filesel_save ? "Save As" : "Open File");
        break;
    case IDE_VIEW_CLI:
        draw_window_title("AmigaDOS CLI");
        break;
    default:
        draw_window_title("GridScript Editor");
        break;
    }
    draw_window_bottom();
    draw_gem_status();
    draw_amiga_header();
    draw_amiga_prompt();
}

static void clear_body(void) {
    for (size_t y = ROW_BODY; y < ROW_BOTTOM; ++y) {
        console_fill_row(y, ' ', GRID_COL_GEM);
    }
}

static void draw_editor_body(void) {
    script_line_t lines[IDE_LINES_MAX];
    int count = parse_source_lines(lines, IDE_LINES_MAX);
    int shown = 0;

    sort_lines(lines, count);
    clear_body();

    for (int i = 0; i < count && shown < (int)ROW_BODY_MAX - 2; ++i) {
        char numbuf[8];
        size_t pos = 0;
        int num = lines[i].number;

        if (num == 0) {
            numbuf[pos++] = '0';
        } else {
            char tmp[8];
            size_t tlen = 0;
            while (num > 0) {
                tmp[tlen++] = (char)('0' + (num % 10));
                num /= 10;
            }
            while (tlen > 0) {
                numbuf[pos++] = tmp[--tlen];
            }
        }
        while (pos < 4) {
            numbuf[pos++] = ' ';
        }
        numbuf[pos] = '\0';

        console_write_at(2, ROW_BODY + (size_t)shown, numbuf, GRID_COL_DIM);
        console_write_at(7, ROW_BODY + (size_t)shown, lines[i].text, GRID_COL_GEM);
        shown++;
    }

    if (count == 0) {
        console_write_at(2, ROW_BODY, "10 PRINT \"Flynn's Grid\"", GRID_COL_DIM);
        console_write_at(2, ROW_BODY + 1, "20 END", GRID_COL_DIM);
    }
}

static void draw_desktop_body(void) {
    clear_body();
    console_write_at(4, ROW_BODY, "[Source]   [Programs] [Flynn]    [Grid]", GRID_COL_OK);
    console_write_at(4, ROW_BODY + 1, " *.grid     ELF bins   archive    logs", GRID_COL_DIM);
    console_write_at(4, ROW_BODY + 3, "[Editor]   [CLI]      [Info]     [Quit]", GRID_COL_OK);
    console_write_at(4, ROW_BODY + 4, " GridScript AmigaDOS  GEM help   End line", GRID_COL_DIM);
    console_write_at(2, ROW_BODY + 6, "Click icons or type 1-8 at AmigaDOS prompt", GRID_COL_AMIGA);
}

static void ide_mouse_cursor_hide(void) {
    if (mouse_cursor_x >= 0 && mouse_cursor_y >= 0) {
        VGA_MEMORY[mouse_cursor_y * 80 + mouse_cursor_x] = mouse_cursor_saved;
        mouse_cursor_x = -1;
        mouse_cursor_y = -1;
    }
}

static void ide_mouse_cursor_tick(void) {
    if (!mouse_present() || current_view != IDE_VIEW_DESKTOP) {
        ide_mouse_cursor_hide();
        return;
    }

    int x = mouse_x();
    int y = mouse_y();

    if (mouse_cursor_x >= 0) {
        VGA_MEMORY[mouse_cursor_y * 80 + mouse_cursor_x] = mouse_cursor_saved;
    }

    mouse_cursor_saved = VGA_MEMORY[y * 80 + x];
    VGA_MEMORY[y * 80 + x] = (uint16_t)0xDB | ((uint16_t)GRID_COL_WARN << 8);
    mouse_cursor_x = x;
    mouse_cursor_y = y;
}

static int desktop_icon_at(int x, int y) {
    if (y >= ROW_BODY && y <= ROW_BODY + 1) {
        if (x >= 4 && x < 14) {
            return 1;
        }
        if (x >= 14 && x < 25) {
            return 2;
        }
        if (x >= 25 && x < 36) {
            return 5;
        }
        if (x >= 36 && x < 47) {
            return 6;
        }
    }

    if (y >= ROW_BODY + 3 && y <= ROW_BODY + 4) {
        if (x >= 4 && x < 14) {
            return 3;
        }
        if (x >= 14 && x < 25) {
            return 4;
        }
        if (x >= 25 && x < 36) {
            return 7;
        }
        if (x >= 36 && x < 47) {
            return 8;
        }
    }

    return 0;
}

static void draw_filesel_body(void) {
    clear_body();
    console_write_at(2, ROW_BODY, "+---- GEM File Selector ---------------------+", GRID_COL_MENU);
    console_write_at(2, ROW_BODY + 1, "| #  Name                                   |", GRID_COL_MENU);

    for (int i = 0; i < filesel_count && i < 8; ++i) {
        char row[64];
        size_t pos = 0;
        row[pos++] = '|';
        row[pos++] = ' ';
        row[pos++] = (char)('1' + i);
        row[pos++] = ' ';
        row[pos++] = ' ';
        copy_string(row + pos, sizeof(row) - pos, basename_from_path(filesel_paths[i]));
        pos = strlen_simple(row);
        while (pos < 43 && pos + 1 < sizeof(row)) {
            row[pos++] = ' ';
        }
        if (pos + 1 < sizeof(row)) {
            row[pos++] = '|';
        }
        row[pos] = '\0';
        console_write_at(2, ROW_BODY + 2 + (size_t)i, row, GRID_COL_GEM);
    }

    console_write_at(2, ROW_BODY + 11, "+-------------------------------------------+", GRID_COL_MENU);
    console_write_at(2, ROW_BODY + 12, "Pick 1-9 or type filename at prompt.", GRID_COL_AMIGA);
}

static void draw_cli_body(void) {
    clear_body();
    console_write_at(0, ROW_BODY, "GridOS AmigaDOS CLI", GRID_COL_AMIGA);
    console_write_at(0, ROW_BODY + 1, "DIR  CD  TYPE  ED  EXEC  ASSIGN  RUN  SAVE", GRID_COL_GEM);
    console_write_at(0, ROW_BODY + 2, "DESKTOP returns to GEM.  QUIT exits Workbench.", GRID_COL_DIM);
}

static void redraw_screen(void) {
    console_clear();
    draw_gem_menu();

    switch (current_view) {
    case IDE_VIEW_DESKTOP:
        draw_window_title("Grid Workbench Desktop");
        draw_desktop_body();
        break;
    case IDE_VIEW_FILESEL:
        draw_window_title(filesel_save ? "Save As" : "Open File");
        draw_filesel_body();
        break;
    case IDE_VIEW_CLI:
        draw_window_title("AmigaDOS CLI");
        draw_cli_body();
        break;
    default:
        draw_window_title("GridScript Editor");
        draw_editor_body();
        break;
    }

    draw_window_bottom();
    draw_gem_status();
    draw_amiga_header();
    draw_amiga_prompt();
}

static void open_filesel(int save_mode) {
    filesel_save = save_mode;
    filesel_count = gfs_list_paths(assign_path, filesel_paths, IDE_FILESEL_MAX);
    if (filesel_count == 0 && !save_mode) {
        filesel_count = gfs_list_paths("/source/", filesel_paths, IDE_FILESEL_MAX);
    }
    current_view = IDE_VIEW_FILESEL;
    redraw_screen();
}

static void show_help(void) {
    current_view = IDE_VIEW_CLI;
    clear_body();
    console_write_at(2, ROW_BODY, "GEM Desk: DESKTOP  EDITOR  FILE OPEN/SAVE", GRID_COL_GEM);
    console_write_at(2, ROW_BODY + 1, "AmigaDOS: DIR CD TYPE ED EXEC ASSIGN RUN", GRID_COL_GEM);
    console_write_at(2, ROW_BODY + 2, "GridScript: 10 PRINT \"text\" / RUN / END", GRID_COL_GEM);
    console_write_at(2, ROW_BODY + 3, "Volumes: source: programs: flynn: grid:", GRID_COL_AMIGA);
    refresh_chrome();
}

static void amiga_dir(const char *arg) {
    char prefix[IDE_PATH_MAX];
    const char *rest = 0;

    if (!arg || *arg == '\0') {
        copy_string(prefix, sizeof(prefix), assign_path);
    } else {
        resolve_volume(arg, &rest, prefix, sizeof(prefix));
    }

    filesel_count = gfs_list_paths(prefix, filesel_paths, IDE_FILESEL_MAX);
    current_view = IDE_VIEW_CLI;
    clear_body();
    console_write_at(0, ROW_BODY, "Directory of ", GRID_COL_AMIGA);
    console_write_at(13, ROW_BODY, prefix, GRID_COL_OK);

    for (int i = 0; i < filesel_count && i < 10; ++i) {
        console_write_at(2, ROW_BODY + 1 + (size_t)i, basename_from_path(filesel_paths[i]), GRID_COL_GEM);
    }

    if (filesel_count == 0) {
        console_write_at(2, ROW_BODY + 1, "(empty)", GRID_COL_DIM);
    }
    refresh_chrome();
}

static void amiga_cd(const char *arg) {
    if (!arg || *arg == '\0') {
        return;
    }

    if (starts_with_ci(arg, "source:")) {
        set_assign("source:", "/source");
    } else if (starts_with_ci(arg, "programs:")) {
        set_assign("programs:", "/programs");
    } else if (starts_with_ci(arg, "flynn:")) {
        set_assign("flynn:", "/flynn");
    } else if (starts_with_ci(arg, "grid:")) {
        set_assign("grid:", "/grid");
    } else if (starts_with_ci(arg, "sys:")) {
        set_assign("sys:", "/");
    } else if (arg[0] == '/') {
        copy_string(assign_path, sizeof(assign_path), arg);
        copy_string(assign_name, sizeof(assign_name), "sys:");
    }
}

static void amiga_type(const char *arg) {
    char path[IDE_PATH_MAX];
    char buf[512];
    size_t len = 0;

    if (!arg) {
        return;
    }

    build_full_path(arg, path, sizeof(path));
    if (gfs_read_file(path, buf, sizeof(buf) - 1, &len) != 0) {
        console_write_at(0, ROW_BODY + 1, "?FILE NOT FOUND", GRID_COL_ERROR);
        return;
    }

    buf[len] = '\0';
    current_view = IDE_VIEW_CLI;
    clear_body();
    console_write_at(0, ROW_BODY, path, GRID_COL_OK);
    console_write_at(0, ROW_BODY + 1, buf, GRID_COL_GEM);
    refresh_chrome();
}

static void amiga_assign(void) {
    current_view = IDE_VIEW_CLI;
    clear_body();
    console_write_at(0, ROW_BODY, "source:   -> /source/", GRID_COL_OK);
    console_write_at(0, ROW_BODY + 1, "programs: -> /programs/", GRID_COL_OK);
    console_write_at(0, ROW_BODY + 2, "flynn:    -> /flynn/", GRID_COL_OK);
    console_write_at(0, ROW_BODY + 3, "grid:     -> /grid/", GRID_COL_OK);
    console_write_at(0, ROW_BODY + 4, "sys:      -> /", GRID_COL_OK);
    refresh_chrome();
}

static int handle_desktop_pick(int n) {
    switch (n) {
    case 1:
        set_assign("source:", "/source");
        open_filesel(0);
        break;
    case 2:
        set_assign("programs:", "/programs");
        amiga_dir(0);
        break;
    case 3:
        current_view = IDE_VIEW_EDITOR;
        redraw_screen();
        break;
    case 4:
        current_view = IDE_VIEW_CLI;
        redraw_screen();
        break;
    case 5:
        set_assign("flynn:", "/flynn");
        amiga_type("motd");
        break;
    case 6:
        set_assign("grid:", "/grid");
        amiga_dir(0);
        break;
    case 7:
        show_help();
        break;
    case 8:
        return 1;
    default:
        break;
    }
    return 0;
}

static void handle_filesel_pick(const char *line) {
    char path[IDE_PATH_MAX];

    if (line[0] >= '1' && line[0] <= '9') {
        int idx = line[0] - '1';
        if (idx >= 0 && idx < filesel_count) {
            if (filesel_save) {
                copy_string(current_path, sizeof(current_path), filesel_paths[idx]);
                ide_save();
            } else {
                ide_load(filesel_paths[idx]);
            }
            current_view = IDE_VIEW_EDITOR;
            redraw_screen();
            return;
        }
    }

    build_full_path(line, path, sizeof(path));
    if (filesel_save) {
        copy_string(current_path, sizeof(current_path), path);
        if (ide_save() == 0) {
            current_view = IDE_VIEW_EDITOR;
            redraw_screen();
        }
        return;
    }

    if (ide_load(path) == 0) {
        current_view = IDE_VIEW_EDITOR;
        redraw_screen();
    }
}

static int handle_amiga_command(const char *line) {
    const char *cmd = line;
    const char *arg = line;

    skip_spaces(&cmd);
    arg = cmd;

    while (*arg && *arg != ' ') {
        arg++;
    }
    if (*arg == ' ') {
        arg++;
        skip_spaces(&arg);
    } else {
        arg = "";
    }

    if (current_view == IDE_VIEW_DESKTOP && cmd[0] >= '1' && cmd[0] <= '8' && cmd[1] == '\0') {
        return handle_desktop_pick(cmd[0] - '0');
    }

    if (current_view == IDE_VIEW_FILESEL) {
        handle_filesel_pick(line);
        return 0;
    }

    if (equals_ci(cmd, "QUIT") || equals_ci(cmd, "BYE") || equals_ci(cmd, "ENDCLI")) {
        return 1;
    }
    if (equals_ci(cmd, "DESKTOP")) {
        current_view = IDE_VIEW_DESKTOP;
        redraw_screen();
        return 0;
    }
    if (equals_ci(cmd, "EDITOR") || equals_ci(cmd, "ED")) {
        if (*arg) {
            char path[IDE_PATH_MAX];
            build_full_path(arg, path, sizeof(path));
            ide_load(path);
        } else {
            current_view = IDE_VIEW_EDITOR;
        }
        redraw_screen();
        return 0;
    }
    if (equals_ci(cmd, "DIR") || equals_ci(cmd, "LIST")) {
        amiga_dir(*arg ? arg : 0);
        return 0;
    }
    if (equals_ci(cmd, "CD")) {
        amiga_cd(arg);
        redraw_screen();
        return 0;
    }
    if (equals_ci(cmd, "TYPE")) {
        amiga_type(arg);
        return 0;
    }
    if (equals_ci(cmd, "EXEC") || equals_ci(cmd, "RUN")) {
        if (*arg) {
            char path[IDE_PATH_MAX];
            build_full_path(arg, path, sizeof(path));
            ide_load(path);
        }
        gridscript_run();
        return 0;
    }
    if (equals_ci(cmd, "SAVE")) {
        if (ide_save() == 0) {
            refresh_chrome();
        }
        return 0;
    }
    if (equals_ci(cmd, "NEW")) {
        ide_new_file();
        current_view = IDE_VIEW_EDITOR;
        redraw_screen();
        return 0;
    }
    if (equals_ci(cmd, "ASSIGN")) {
        amiga_assign();
        return 0;
    }
    if (equals_ci(cmd, "OPEN")) {
        open_filesel(0);
        return 0;
    }
    if (equals_ci(cmd, "SAVEAS")) {
        open_filesel(1);
        return 0;
    }
    if (equals_ci(cmd, "HELP") || equals_ci(cmd, "?")) {
        show_help();
        return 0;
    }
    if (equals_ci(cmd, "CLI")) {
        current_view = IDE_VIEW_CLI;
        redraw_screen();
        return 0;
    }

    if (current_view == IDE_VIEW_EDITOR) {
        const char *cursor = cmd;
        int number = 0;

        if (parse_line_number(&cursor, &number)) {
            skip_spaces(&cursor);
            if (*cursor == '\0') {
                ide_delete_line(number);
            } else {
                ide_set_line(number, cursor);
            }
            redraw_screen();
            return 0;
        }
    }

    console_write_at(2, ROW_BODY + 1, "?Unknown command", GRID_COL_ERROR);
    refresh_chrome();
    return 0;
}

static int handle_function_key_num(int fkey) {
    if (fkey == 1) {
        show_help();
        return 1;
    }
    if (fkey == 2) {
        open_filesel(1);
        return 1;
    }
    if (fkey == 3) {
        open_filesel(0);
        return 1;
    }
    if (fkey == 4) {
        if (ide_save() == 0) {
            refresh_chrome();
        }
        return 1;
    }
    if (fkey == 5) {
        gridscript_run();
        return 1;
    }
    if (fkey == 6) {
        current_view = IDE_VIEW_DESKTOP;
        redraw_screen();
        return 1;
    }
    if (fkey == 10) {
        return 2;
    }
    return 0;
}

static int handle_function_key(const char *line) {
    if (line[0] != 'F' && line[0] != 'f') {
        return 0;
    }

    int fkey = 0;
    const char *digits = line + 1;
    while (*digits >= '0' && *digits <= '9') {
        fkey = fkey * 10 + (*digits - '0');
        digits++;
    }

    if (fkey <= 0) {
        return 0;
    }

    return handle_function_key_num(fkey);
}

void ide_run(const char *initial_path) {
    char line[IDE_LINE_MAX];
    int exit_requested = 0;

    if (!security_require_capability(CAP_WRITE_GRID, "ide")) {
        return;
    }

    ide_new_file();
    set_assign("source:", "/source");

    if (initial_path && initial_path[0] != '\0') {
        if (ide_load(initial_path) != 0) {
            copy_string(current_path, sizeof(current_path), initial_path);
        }
    } else if (gfs_present()) {
        if (ide_load("/source/welcome.grid") != 0) {
            ide_new_file();
        }
    }

    current_view = IDE_VIEW_EDITOR;
    redraw_screen();
    log_event("workbench opened");
    console_set_idle_tick(ide_mouse_cursor_tick);

    while (!exit_requested) {
        console_input_t input;

        console_read_prompt(&input, PROMPT_COL, ROW_PROMPT, GRID_COL_GEM, sizeof(input.line));

        if (input.type == CON_INPUT_MOUSE) {
            if (current_view == IDE_VIEW_DESKTOP) {
                int icon = desktop_icon_at(input.mouse_x, input.mouse_y);
                if (icon > 0 && handle_desktop_pick(icon)) {
                    exit_requested = 1;
                }
            }
            continue;
        }

        if (input.type == CON_INPUT_FKEY) {
            int fk = handle_function_key_num(input.fkey);
            if (fk == 2) {
                exit_requested = 1;
            }
            continue;
        }

        if (input.type == CON_INPUT_ESC) {
            current_view = IDE_VIEW_DESKTOP;
            redraw_screen();
            continue;
        }

        copy_string(line, sizeof(line), input.line);

        if (line[0] == '\0') {
            continue;
        }

        int fk = handle_function_key(line);
        if (fk == 2) {
            exit_requested = 1;
            continue;
        }
        if (fk == 1) {
            continue;
        }

        if (starts_with_ci(line, "FILE ")) {
            const char *sub = line + 5;
            skip_spaces(&sub);
            if (starts_with_ci(sub, "OPEN")) {
                open_filesel(0);
                continue;
            }
            if (starts_with_ci(sub, "SAVE")) {
                const char *save_arg = sub + 4;
                skip_spaces(&save_arg);
                if (starts_with_ci(save_arg, "AS")) {
                    open_filesel(1);
                } else if (ide_save() == 0) {
                    refresh_chrome();
                }
                continue;
            }
        }

        if (handle_amiga_command(line)) {
            exit_requested = 1;
        }
    }

    if (modified) {
        console_set_color(GRID_COL_WARN);
        console_write_line("Workbench closed — unsaved changes lost.");
        console_set_color(GRID_COL_DEFAULT);
    }

    log_event("workbench closed");
    console_set_idle_tick(0);
    ide_mouse_cursor_hide();
    console_reset_cursor(0, CONSOLE_ROWS - 1);
}
