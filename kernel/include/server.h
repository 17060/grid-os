#ifndef SERVER_H
#define SERVER_H

#include <stddef.h>
#include <stdint.h>

#define GRID_SERVER_SLOTS      8
#define GRID_SERVER_LISTEN_MAX 4
#define GRID_SERVER_LINE_MAX   256

void grid_server_init(void);
int grid_server_listen(uint16_t port);
int grid_server_unlisten(uint16_t port);
void grid_server_stop_all(void);
void grid_server_poll(void);
int grid_server_accept(void);
int grid_server_read_line(int slot, char *line, size_t cap);
int grid_server_write(int slot, const char *text);
int grid_server_reply(int slot, const char *text);
void grid_server_close(int slot);
int grid_server_dispatch_builtin(int slot, const char *line);
void grid_server_format_status(char *out, size_t cap);
int grid_server_listening(uint16_t port);

#endif
