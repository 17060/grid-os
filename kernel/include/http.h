#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>
#include <stdint.h>

/* Minimal HTTP/1.1 GET over the Grid TCP stack. Returns body bytes written. */
int http_get(uint32_t ip, uint16_t port, const char *path, char *out, size_t cap);
int http_get_host(const char *host, uint16_t port, const char *path, char *out, size_t cap);
void http_close_idle(void);

#endif
