#include "vbe.h"

#include "memory.h"

#include <stddef.h>
#include <stdint.h>

extern const uint8_t font8x16[96][16];

#define VBE_DISPI_INDEX_ID           0x0
#define VBE_DISPI_INDEX_XRES         0x1
#define VBE_DISPI_INDEX_YRES         0x2
#define VBE_DISPI_INDEX_BPP          0x3
#define VBE_DISPI_INDEX_ENABLE       0x4
#define VBE_DISPI_INDEX_BANK         0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH   0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT  0x7
#define VBE_DISPI_INDEX_X_OFFSET     0x8
#define VBE_DISPI_INDEX_Y_OFFSET     0x9

#define VBE_DISPI_ID0                0xB0C0
#define VBE_DISPI_ID1                0xB0C1
#define VBE_DISPI_ID2                0xB0C2
#define VBE_DISPI_ID3                0xB0C3
#define VBE_DISPI_ID4                0xB0C4
#define VBE_DISPI_ID5                0xB0C5

#define VBE_DISPI_ENABLED            0x01
#define VBE_DISPI_GETCAPS            0x02
#define VBE_DISPI_LFB_ENABLED        0x40

#define VBE_DISPI_LFB_PHYS           0xE0000000ULL
#define VBE_FB_SIZE                  (VBE_WIDTH * VBE_HEIGHT * 4u)

#define VBE_COLS 80u
#define VBE_ROWS 25u
#define VBE_CELL_W (VBE_WIDTH / VBE_COLS)
#define VBE_CELL_H (VBE_HEIGHT / VBE_ROWS)

static volatile uint32_t *fb;
static int active;
static char mode_str[48];
static char shadow_c[VBE_ROWS][VBE_COLS];
static uint8_t shadow_a[VBE_ROWS][VBE_COLS];

static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t value;
    __asm__ volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static uint16_t vbe_read(uint16_t index) {
    outw(0x01CE, index);
    return inw(0x01CF);
}

static void vbe_write(uint16_t index, uint16_t value) {
    outw(0x01CE, index);
    outw(0x01CF, value);
}

static int vbe_probe(void) {
    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_GETCAPS);
    uint16_t id = vbe_read(VBE_DISPI_INDEX_ID);
    return id >= VBE_DISPI_ID0 && id <= VBE_DISPI_ID5;
}

static uint32_t vga_color(uint8_t attr, int fg) {
    static const uint32_t pal[16] = {
        0x000000, 0x0000AA, 0x00AA00, 0x00AAAA,
        0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
        0x555555, 0x5555FF, 0x55FF55, 0x55FFFF,
        0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF,
    };
    uint8_t idx = fg ? (attr & 0x0F) : ((attr >> 4) & 0x0F);
    return pal[idx & 0x0F];
}

static void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!fb || x >= VBE_WIDTH || y >= VBE_HEIGHT) {
        return;
    }
    fb[y * VBE_WIDTH + x] = color;
}

static const uint8_t *glyph_for(char c) {
    if (c >= 32 && c <= 127) {
        return font8x16[(size_t)c - 32];
    }
    return font8x16[0];
}

static void draw_glyph_scaled(uint32_t px, uint32_t py, char c, uint32_t fg, uint32_t bg) {
    const uint8_t *glyph = glyph_for(c);
    uint32_t sx = VBE_CELL_W / 8u;
    if (sx < 1u) {
        sx = 1u;
    }
    uint32_t sy = VBE_CELL_H / 16u;
    if (sy < 1u) {
        sy = 1u;
    }

    for (uint32_t row = 0; row < VBE_CELL_H; ++row) {
        uint32_t gy = row / sy;
        if (gy > 15u) {
            gy = 15u;
        }
        uint8_t bits = glyph[gy];
        for (uint32_t col = 0; col < VBE_CELL_W; ++col) {
            uint32_t gx = col / sx;
            if (gx > 7u) {
                gx = 7u;
            }
            uint32_t color = (bits & (0x80u >> gx)) ? fg : bg;
            fb_put_pixel(px + col, py + row, color);
        }
    }
}

void vbe_fill_cell(size_t col, size_t row, char c, uint8_t attr) {
    if (!active || !fb || col >= VBE_COLS || row >= VBE_ROWS) {
        return;
    }
    shadow_c[row][col] = c;
    shadow_a[row][col] = attr;
    uint32_t px = (uint32_t)(col * VBE_CELL_W);
    uint32_t py = (uint32_t)(row * VBE_CELL_H);
    uint32_t fg = vga_color(attr, 1);
    uint32_t bg = vga_color(attr, 0);
    if (c == ' ') {
        for (uint32_t y = 0; y < VBE_CELL_H; ++y) {
            for (uint32_t x = 0; x < VBE_CELL_W; ++x) {
                fb_put_pixel(px + x, py + y, bg);
            }
        }
        return;
    }
    draw_glyph_scaled(px, py, c, fg, bg);
}

void vbe_scroll_up(void) {
    if (!active || !fb) {
        return;
    }
    for (size_t y = 1; y < VBE_ROWS; ++y) {
        for (size_t x = 0; x < VBE_COLS; ++x) {
            shadow_c[y - 1][x] = shadow_c[y][x];
            shadow_a[y - 1][x] = shadow_a[y][x];
        }
    }
    for (size_t x = 0; x < VBE_COLS; ++x) {
        shadow_c[VBE_ROWS - 1][x] = ' ';
        shadow_a[VBE_ROWS - 1][x] = 0x07;
    }
    for (size_t y = 0; y < VBE_ROWS; ++y) {
        for (size_t x = 0; x < VBE_COLS; ++x) {
            vbe_fill_cell(x, y, shadow_c[y][x], shadow_a[y][x]);
        }
    }
}

void vbe_put_cell(size_t col, size_t row, char c, uint8_t attr) {
    vbe_fill_cell(col, row, c, attr);
}

void vbe_clear(uint8_t attr) {
    if (!active || !fb) {
        return;
    }
    for (size_t y = 0; y < VBE_ROWS; ++y) {
        for (size_t x = 0; x < VBE_COLS; ++x) {
            shadow_c[y][x] = ' ';
            shadow_a[y][x] = attr;
        }
    }
    uint32_t bg = vga_color(attr, 0);
    for (uint32_t i = 0; i < VBE_WIDTH * VBE_HEIGHT; ++i) {
        fb[i] = bg;
    }
}

int vbe_is_active(void) {
    return active;
}

const char *vbe_mode_string(void) {
    return mode_str;
}

int vbe_init_4k(void) {
    if (!vbe_probe()) {
        mode_str[0] = '\0';
        return -1;
    }

    if (memory_map_kernel_phys(VBE_DISPI_LFB_PHYS, VBE_FB_SIZE) != 0) {
        mode_str[0] = '\0';
        return -1;
    }

    fb = (volatile uint32_t *)(uintptr_t)VBE_DISPI_LFB_PHYS;

    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_GETCAPS);
    vbe_write(VBE_DISPI_INDEX_XRES, (uint16_t)VBE_WIDTH);
    vbe_write(VBE_DISPI_INDEX_YRES, (uint16_t)VBE_HEIGHT);
    vbe_write(VBE_DISPI_INDEX_BPP, 32);
    vbe_write(VBE_DISPI_INDEX_BANK, 0);
    vbe_write(VBE_DISPI_INDEX_VIRT_WIDTH, (uint16_t)VBE_WIDTH);
    vbe_write(VBE_DISPI_INDEX_VIRT_HEIGHT, (uint16_t)VBE_HEIGHT);
    vbe_write(VBE_DISPI_INDEX_X_OFFSET, 0);
    vbe_write(VBE_DISPI_INDEX_Y_OFFSET, 0);
    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);

    active = 1;
    for (size_t y = 0; y < VBE_ROWS; ++y) {
        for (size_t x = 0; x < VBE_COLS; ++x) {
            shadow_c[y][x] = ' ';
            shadow_a[y][x] = 0x07;
        }
    }
    vbe_clear(0x07);

    const char *tag = "Grid OS 4K HDMI 3840x2160 — GridBASIC IDE";
    uint8_t attr = 0x0B;
    for (size_t i = 0; tag[i]; ++i) {
        vbe_put_cell(i, 0, tag[i], attr);
    }

    const char *out = "4K framebuffer 3840x2160";
    size_t n = 0;
    while (out[n] && n + 1 < sizeof(mode_str)) {
        mode_str[n] = out[n];
        n++;
    }
    mode_str[n] = '\0';
    return 0;
}
