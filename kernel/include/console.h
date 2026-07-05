#ifndef CONSOLE_H
#define CONSOLE_H

#include <stddef.h>
#include <stdint.h>

#define GRID_COL_DEFAULT 0x0B
#define GRID_COL_DIM     0x03
#define GRID_COL_WARN    0x0E
#define GRID_COL_ERROR   0x0C
#define GRID_COL_OK      0x0A
#define GRID_COL_TITLE   0x1F
#define GRID_COL_MENU    0x70
#define GRID_COL_AMIGA   0x0E
#define GRID_COL_GEM     0x07

#define CONSOLE_ROWS     25
#define CONSOLE_COLS     80

#define CON_INPUT_LINE   0
#define CON_INPUT_FKEY   1
#define CON_INPUT_ESC    2
#define CON_INPUT_MOUSE  3

typedef struct {
    int type;
    char line[128];
    int fkey;
    int mouse_x;
    int mouse_y;
} console_input_t;

#define CONSOLE_CTRL_C 0x03
#define CONSOLE_SC_UP    0x100
#define CONSOLE_SC_DOWN  0x101
#define CONSOLE_SC_LEFT  0x102
#define CONSOLE_SC_RIGHT 0x103
#define CONSOLE_SC_HOME  0x104
#define CONSOLE_SC_END   0x105
#define CONSOLE_SC_DEL   0x106

void console_init(void);
void console_clear(void);
void console_set_color(uint8_t color);
void console_write(const char *text);
void console_write_line(const char *text);
void console_write_char(char c);
void console_write_at(size_t x, size_t y, const char *text, uint8_t attr);
void console_fill_row(size_t y, char c, uint8_t attr);
void console_reset_cursor(size_t x, size_t y);
void console_set_idle_tick(void (*fn)(void));
void console_set_serial_mirror(int enabled);
char console_read_char(void);
int console_try_read_scancode(void);
int console_read_scancode(void);
/* Returns a translated character (0..255, including '\n','\b','\t',Esc=27)
 * or a CONSOLE_SC_* value (>=0x100) for arrow/home/end/del keys. Blocks. */
int console_read_key(void);
void console_read_line(char *buffer, size_t capacity);
void console_read_line_hist(char *buffer, size_t capacity, char history[][128], int history_max,
                            int *history_count);
void console_read_line_at(char *buffer, size_t capacity, size_t x, size_t y, uint8_t attr);
void console_read_prompt(console_input_t *out, size_t x, size_t y, uint8_t attr, size_t capacity);

#endif
