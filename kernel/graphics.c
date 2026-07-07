#include "graphics.h"

#define VGA_W 80
#define VGA_H 25
#define VGA_MEM ((volatile uint16_t *)0xB8000)

static uint16_t g_entry(char c, uint8_t attr) {
    return (uint16_t)c | ((uint16_t)attr << 8);
}

static void plot_raw(int x, int y, uint8_t color) {
    if (x < 0 || y < 0 || x >= VGA_W || y >= VGA_H) {
        return;
    }
    char ch = VGA_MEM[y * VGA_W + x] & 0xFF;
    if (ch == ' ') {
        ch = 0xDB;
    }
    VGA_MEM[y * VGA_W + x] = g_entry(ch, color);
}

void grid_plot(int x, int y, uint8_t color) {
    plot_raw(x, y, color);
}

void grid_line(int x0, int y0, int x1, int y1, uint8_t color) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    if (dx < 0) {
        dx = -dx;
    }
    if (dy < 0) {
        dy = -dy;
    }
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    for (;;) {
        grid_plot(x0, y0, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        int e2 = err * 2;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void grid_circle(int cx, int cy, int r, uint8_t color) {
    if (r < 1) {
        r = 1;
    }
    int x = r;
    int y = 0;
    int err = 0;
    while (x >= y) {
        grid_plot(cx + x, cy + y, color);
        grid_plot(cx + y, cy + x, color);
        grid_plot(cx - y, cy + x, color);
        grid_plot(cx - x, cy + y, color);
        grid_plot(cx - x, cy - y, color);
        grid_plot(cx - y, cy - x, color);
        grid_plot(cx + y, cy - x, color);
        grid_plot(cx + x, cy - y, color);
        if (err <= 0) {
            y++;
            err += y * 2 + 1;
        }
        if (err > 0) {
            x--;
            err -= x * 2 + 1;
        }
    }
}

void grid_plot_clear(void) {
    for (int y = 0; y < VGA_H; ++y) {
        for (int x = 0; x < VGA_W; ++x) {
            VGA_MEM[y * VGA_W + x] = g_entry(' ', 0x07);
        }
    }
}
