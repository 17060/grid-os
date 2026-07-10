#ifndef VBE_H
#define VBE_H

#include <stddef.h>
#include <stdint.h>

#define VBE_WIDTH  3840u
#define VBE_HEIGHT 2160u

int vbe_init_4k(void);
int vbe_is_active(void);
void vbe_put_cell(size_t col, size_t row, char c, uint8_t attr);
void vbe_fill_cell(size_t col, size_t row, char c, uint8_t attr);
void vbe_clear(uint8_t attr);
void vbe_scroll_up(void);

#endif
