#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

void mouse_init(void);
int mouse_present(void);
void mouse_poll(void);
int mouse_moved(void);
int mouse_clicked(void);
uint8_t mouse_buttons(void);
int mouse_x(void);
int mouse_y(void);
void mouse_consume_click(void);

#endif
