#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

void grid_plot(int x, int y, uint8_t color);
void grid_line(int x0, int y0, int x1, int y1, uint8_t color);
void grid_circle(int cx, int cy, int r, uint8_t color);
void grid_plot_clear(void);

#endif
