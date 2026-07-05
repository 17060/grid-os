#ifndef SERIAL_H
#define SERIAL_H

#include <stddef.h>
#include <stdint.h>

#define COM1_PORT 0x3F8

void serial_init(void);
int serial_is_online(void);
int serial_can_read(void);
int serial_read_byte(void);
void serial_write_byte(char byte);
void serial_write(const char *text);
size_t serial_read_line(char *buffer, size_t capacity, uint32_t spin_limit);

#endif
