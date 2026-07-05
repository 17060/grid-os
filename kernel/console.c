#include "console.h"
#include "grid.h"
#include "mouse.h"
#include "net.h"
#include "sched.h"
#include "security.h"
#include "serial.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t *)0xB8000)

static size_t row = 0;
static size_t col = 0;
static uint8_t color = GRID_COL_DEFAULT;
static void (*idle_tick_fn)(void);
static int shift_down = 0;
static int ctrl_down = 0;
static int serial_mirror = 0;

static uint16_t make_entry(char c, uint8_t attr) {
    return (uint16_t)c | ((uint16_t)attr << 8);
}

static void scroll(void) {
    for (size_t y = 1; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            VGA_MEMORY[(y - 1) * VGA_WIDTH + x] = VGA_MEMORY[y * VGA_WIDTH + x];
        }
    }

    for (size_t x = 0; x < VGA_WIDTH; ++x) {
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = make_entry(' ', color);
    }

    if (row > 0) {
        row--;
    }
}

static void put_at(char c, uint8_t attr, size_t x, size_t y) {
    VGA_MEMORY[y * VGA_WIDTH + x] = make_entry(c, attr);
}

static void mirror_char(char c) {
    if (!serial_mirror || !serial_is_online() || c == '\0') {
        return;
    }
    if (c == '\n') {
        serial_write_byte('\r');
    }
    serial_write_byte(c);
}

void console_set_serial_mirror(int enabled) {
    serial_mirror = enabled ? 1 : 0;
}

void console_init(void) {
    console_clear();
}

void console_clear(void) {
    row = 0;
    col = 0;
    color = GRID_COL_DEFAULT;

    for (size_t y = 0; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            put_at(' ', color, x, y);
        }
    }
}

void console_set_color(uint8_t new_color) {
    color = new_color;
}

void console_write_char(char c) {
    if (c == '\n') {
        col = 0;
        if (++row == VGA_HEIGHT) {
            scroll();
        }
        mirror_char(c);
        return;
    }

    put_at(c, color, col, row);
    mirror_char(c);
    if (++col == VGA_WIDTH) {
        col = 0;
        if (++row == VGA_HEIGHT) {
            scroll();
        }
    }
}

void console_write(const char *text) {
    while (*text) {
        console_write_char(*text++);
    }
}

void console_write_line(const char *text) {
    console_write(text);
    console_write_char('\n');
}

void console_write_at(size_t x, size_t y, const char *text, uint8_t attr) {
    size_t cx = x;
    while (*text && cx < VGA_WIDTH && y < VGA_HEIGHT) {
        put_at(*text++, attr, cx, y);
        cx++;
    }
}

void console_fill_row(size_t y, char c, uint8_t attr) {
    if (y >= VGA_HEIGHT) {
        return;
    }
    for (size_t x = 0; x < VGA_WIDTH; ++x) {
        put_at(c, attr, x, y);
    }
}

void console_reset_cursor(size_t x, size_t y) {
    if (x >= VGA_WIDTH) {
        x = VGA_WIDTH - 1;
    }
    if (y >= VGA_HEIGHT) {
        y = VGA_HEIGHT - 1;
    }
    row = y;
    col = x;
    color = GRID_COL_DEFAULT;
}

static int ps2_has_data(void) {
    uint8_t status;
    __asm__ volatile("inb %1, %0" : "=a"(status) : "Nd"(0x64));
    return (status & 0x01) != 0;
}

void console_set_idle_tick(void (*fn)(void)) {
    idle_tick_fn = fn;
}

static void console_idle(void) {
    mouse_poll();
    if (idle_tick_fn) {
        idle_tick_fn();
    }
    net_poll();
    if (!ps2_has_data()) {
        sched_service();
    }
}

static uint8_t ps2_read(void) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(0x60));
    return value;
}

#include <stdint.h>

static int scancode_to_fkey(uint8_t scancode) {
    if (scancode >= 0x3Bu && scancode <= 0x44u) {
        return (int)(scancode - 0x3Au);
    }
    return 0;
}

static char translate_scancode(uint8_t scancode) {
    static const char map[128] = {
        0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
        '*', 0, ' '
    };

    if (scancode >= 128) {
        return 0;
    }

    char c = map[scancode];
    if (ctrl_down && (c == 'c' || c == 'C')) {
        return (char)CONSOLE_CTRL_C;
    }

    if (shift_down && c >= 'a' && c <= 'z') {
        c = (char)(c - 'a' + 'A');
    } else if (shift_down) {
        switch (c) {
        case '1':
            return '!';
        case '2':
            return '@';
        case '3':
            return '#';
        case '4':
            return '$';
        case '5':
            return '%';
        case '6':
            return '^';
        case '7':
            return '&';
        case '8':
            return '*';
        case '9':
            return '(';
        case '0':
            return ')';
        case '-':
            return '_';
        case '=':
            return '+';
        case '[':
            return '{';
        case ']':
            return '}';
        case ';':
            return ':';
        case '\'':
            return '"';
        case '`':
            return '~';
        case '\\':
            return '|';
        case ',':
            return '<';
        case '.':
            return '>';
        case '/':
            return '?';
        default:
            break;
        }
    }

    return c;
}

static void handle_shift_scancode(uint8_t scancode, int pressed) {
    if (scancode == 0x2Au || scancode == 0x36u) {
        shift_down = pressed;
    }
}

static void handle_ctrl_scancode(uint8_t scancode, int pressed) {
    if (scancode == 0x1Du) {
        ctrl_down = pressed;
    }
}

int console_try_read_scancode(void) {
    console_idle();
    if (!ps2_has_data()) {
        return -1;
    }

    uint8_t scancode = ps2_read();
    if (scancode == 0xE0u) {
        console_idle();
        if (!ps2_has_data()) {
            return -1;
        }
        uint8_t ext = ps2_read();
        if (ext == 0x48u) {
            return CONSOLE_SC_UP;
        }
        if (ext == 0x50u) {
            return CONSOLE_SC_DOWN;
        }
        if (ext == 0x4Bu) {
            return CONSOLE_SC_LEFT;
        }
        if (ext == 0x4Du) {
            return CONSOLE_SC_RIGHT;
        }
        if (ext == 0x52u) {
            return CONSOLE_SC_DEL;
        }
        if (ext == 0x47u) {
            return CONSOLE_SC_HOME;
        }
        if (ext == 0x4Fu) {
            return CONSOLE_SC_END;
        }
        return -1;
    }
    if (scancode & 0x80) {
        uint8_t code = (uint8_t)(scancode & 0x7Fu);
        handle_shift_scancode(code, 0);
        handle_ctrl_scancode(code, 0);
        return -1;
    }

    handle_shift_scancode(scancode, 1);
    handle_ctrl_scancode(scancode, 1);
    if (scancode == 0x1Du || scancode == 0x2Au || scancode == 0x36u) {
        return -1;
    }
    return (int)scancode;
}

static int try_read_serial_char(char *out) {
    if (!serial_can_read()) {
        return 0;
    }

    int value = serial_read_byte();
    if (value < 0) {
        return 0;
    }

    char c = (char)value;
    if (c == '\r') {
        return 0;
    }
    if (c == (char)CONSOLE_CTRL_C) {
        *out = (char)CONSOLE_CTRL_C;
        return 1;
    }
    if (c == 127) {
        c = '\b';
    }

    *out = c;
    return 1;
}

/* After a serial ESC byte, look for an ANSI escape sequence (ESC [ A etc.)
 * so arrow keys work over -serial stdio. Returns a CONSOLE_SC_* code, or 0
 * if no sequence followed (bare Esc keypress). */
static int serial_try_read_ansi(void) {
    uint32_t spins = 0;
    int b = -1;
    while (spins < 200000u) {
        b = serial_read_byte();
        if (b >= 0) {
            break;
        }
        spins++;
    }
    if (b != '[') {
        return 0;
    }
    spins = 0;
    b = -1;
    while (spins < 200000u) {
        b = serial_read_byte();
        if (b >= 0) {
            break;
        }
        spins++;
    }
    switch (b) {
    case 'A': return CONSOLE_SC_UP;
    case 'B': return CONSOLE_SC_DOWN;
    case 'C': return CONSOLE_SC_RIGHT;
    case 'D': return CONSOLE_SC_LEFT;
    case 'H': return CONSOLE_SC_HOME;
    case 'F': return CONSOLE_SC_END;
    case '3':
        spins = 0;
        while (spins < 200000u) {
            int t = serial_read_byte();
            if (t >= 0) {
                break;
            }
            spins++;
        }
        return CONSOLE_SC_DEL;
    default:
        return 0;
    }
}

char console_read_char(void) {
    for (;;) {
        int scancode = console_try_read_scancode();
        if (scancode >= 0) {
            uint8_t code = (uint8_t)scancode;
            if (code == 0x2Au || code == 0x36u || code == 0x1Du) {
                continue;
            }

            char c = translate_scancode(code);
            if (c == (char)CONSOLE_CTRL_C) {
                return (char)CONSOLE_CTRL_C;
            }
            if (c == '\b') {
                return c;
            }
            if (c != 0) {
                return c;
            }
            continue;
        }

        char serial_c;
        if (try_read_serial_char(&serial_c)) {
            return serial_c;
        }
    }
}

int console_read_scancode(void) {
    for (;;) {
        int scancode = console_try_read_scancode();
        if (scancode >= 0) {
            return scancode;
        }
    }
}

int console_read_key(void) {
    for (;;) {
        int scancode = console_try_read_scancode();
        if (scancode >= 0) {
            if (scancode >= 0x100) {
                return scancode;   /* special key */
            }
            uint8_t code = (uint8_t)scancode;
            if (code == 0x2Au || code == 0x36u || code == 0x1Du) {
                continue;
            }
            char c = translate_scancode(code);
            if (c == 0) {
                continue;
            }
            return (int)(unsigned char)c;
        }

        char serial_c;
        if (try_read_serial_char(&serial_c)) {
            if (serial_c == 27) {
                int seq = serial_try_read_ansi();
                if (seq != 0) {
                    return seq;
                }
                return 27;
            }
            return (int)(unsigned char)serial_c;
        }
    }
}

void console_read_prompt(console_input_t *out, size_t x, size_t y, uint8_t attr, size_t capacity) {
    size_t length = 0;

    if (!out || capacity == 0 || capacity > sizeof(out->line) || y >= VGA_HEIGHT) {
        return;
    }

    out->type = CON_INPUT_LINE;
    out->fkey = 0;
    out->line[0] = '\0';

    for (;;) {
        if (mouse_clicked()) {
            out->type = CON_INPUT_MOUSE;
            out->mouse_x = mouse_x();
            out->mouse_y = mouse_y();
            out->fkey = 0;
            out->line[0] = '\0';
            mouse_consume_click();
            return;
        }

        int scancode = console_try_read_scancode();
        if (scancode < 0) {
            continue;
        }

        int fkey = scancode_to_fkey((uint8_t)scancode);

        if (fkey > 0) {
            out->type = CON_INPUT_FKEY;
            out->fkey = fkey;
            out->line[0] = '\0';
            return;
        }

        if (scancode == 0x01) {
            out->type = CON_INPUT_ESC;
            out->line[0] = '\0';
            return;
        }

        if (scancode == 0x2A || scancode == 0x36) {
            continue;
        }

        char c = translate_scancode((uint8_t)scancode);
        if (c == '\n') {
            out->type = CON_INPUT_LINE;
            out->line[length] = '\0';
            console_reset_cursor(x + length, y + 1);
            return;
        }

        if (c == '\b') {
            if (length > 0) {
                length--;
                out->line[length] = '\0';
                put_at(' ', attr, x + length, y);
            }
            continue;
        }

        if (c == 0) {
            continue;
        }

        if (length + 1 >= capacity) {
            continue;
        }

        out->line[length++] = c;
        out->line[length] = '\0';
        put_at(c, attr, x + length - 1, y);
    }
}

void console_read_line_hist(char *buffer, size_t capacity, char history[][128], int history_max,
                            int *history_count) {
    size_t length = 0;
    int browse = history_count ? *history_count : 0;

    if (capacity == 0) {
        return;
    }

    buffer[0] = '\0';

    for (;;) {
        int scancode = console_try_read_scancode();
        if (scancode >= 0 && scancode < 0x100) {
            if (scancode == 0x2Au || scancode == 0x36u || scancode == 0x1Du) {
                continue;
            }

            char c = translate_scancode((uint8_t)scancode);
            if (c == (char)CONSOLE_CTRL_C) {
                console_write("^C");
                console_write_char('\n');
                buffer[0] = '\0';
                return;
            }
            if (c == '\n') {
                console_write_char('\n');
                buffer[length] = '\0';
                return;
            }
            if (c == '\b') {
                if (length > 0) {
                    length--;
                    buffer[length] = '\0';
                    if (col > 0) {
                        col--;
                    }
                    put_at(' ', color, col, row);
                }
                continue;
            }
            if (c == 0) {
                continue;
            }
            if (length + 1 >= capacity) {
                continue;
            }
            buffer[length++] = c;
            buffer[length] = '\0';
            console_write_char(c);
            continue;
        }

        if (scancode == CONSOLE_SC_UP || scancode == CONSOLE_SC_DOWN) {
            if (!history || !history_count || *history_count == 0 || history_max <= 0) {
                continue;
            }

            if (scancode == CONSOLE_SC_UP) {
                if (browse > 0) {
                    browse--;
                }
            } else if (browse < *history_count) {
                browse++;
            }

            while (length > 0) {
                length--;
                if (col > 0) {
                    col--;
                }
                put_at(' ', color, col, row);
            }
            buffer[0] = '\0';

            if (browse < *history_count) {
                size_t i = 0;
                while (history[browse][i] && i + 1 < capacity) {
                    buffer[i] = history[browse][i];
                    i++;
                }
                buffer[i] = '\0';
                length = i;
                for (size_t j = 0; j < length; ++j) {
                    console_write_char(buffer[j]);
                }
            }
            continue;
        }

        char serial_c;
        if (try_read_serial_char(&serial_c)) {
            if (serial_c == (char)CONSOLE_CTRL_C) {
                console_write("^C");
                console_write_char('\n');
                buffer[0] = '\0';
                return;
            }
            if (serial_c == '\n') {
                console_write_char('\n');
                buffer[length] = '\0';
                return;
            }
            if (serial_c == '\b') {
                if (length > 0) {
                    length--;
                    buffer[length] = '\0';
                    if (col > 0) {
                        col--;
                    }
                    put_at(' ', color, col, row);
                }
                continue;
            }
            if (length + 1 >= capacity) {
                continue;
            }
            buffer[length++] = serial_c;
            buffer[length] = '\0';
            console_write_char(serial_c);
        }
    }
}

void console_read_line(char *buffer, size_t capacity) {
    size_t length = 0;

    if (capacity == 0) {
        return;
    }

    buffer[0] = '\0';

    for (;;) {
        char c = console_read_char();

        if (c == (char)CONSOLE_CTRL_C) {
            console_write("^C");
            console_write_char('\n');
            buffer[0] = '\0';
            return;
        }

        if (c == '\n') {
            console_write_char('\n');
            buffer[length] = '\0';
            return;
        }

        if (c == '\b') {
            if (length > 0) {
                length--;
                buffer[length] = '\0';
                if (col > 0) {
                    col--;
                }
                put_at(' ', color, col, row);
            }
            continue;
        }

        if (length + 1 >= capacity) {
            continue;
        }

        buffer[length++] = c;
        buffer[length] = '\0';
        console_write_char(c);
    }
}

void console_read_line_at(char *buffer, size_t capacity, size_t x, size_t y, uint8_t attr) {
    size_t length = 0;

    if (capacity == 0 || y >= VGA_HEIGHT) {
        return;
    }

    buffer[0] = '\0';

    for (;;) {
        char c = console_read_char();

        if (c == '\n') {
            console_reset_cursor(x + length, y + 1);
            return;
        }

        if (c == '\b') {
            if (length > 0) {
                length--;
                buffer[length] = '\0';
                put_at(' ', attr, x + length, y);
            }
            continue;
        }

        if (length + 1 >= capacity) {
            continue;
        }

        buffer[length++] = c;
        buffer[length] = '\0';
        put_at(c, attr, x + length - 1, y);
    }
}
