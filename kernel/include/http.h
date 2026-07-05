#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>
#include <stdint.h>

/* Minimal HTTP/1.0 GET over the Grid TCP stack. Returns body bytes written. */
int http_get(uint32_t ip, uint16_t port, const char *path, char *out, size_t cap);

#endif
