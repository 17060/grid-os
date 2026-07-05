#include "http.h"
#include "net.h"
#include "tcp.h"

#include <stddef.h>
#include <stdint.h>

static size_t scopy(char *dst, size_t cap, const char *src) {
    size_t i = 0;
    if (src) {
        while (src[i] && i + 1 < cap) {
            dst[i] = src[i];
            i++;
        }
    }
    dst[i] = '\0';
    return i;
}

static size_t append(char *dst, size_t cap, size_t pos, const char *src) {
    while (*src && pos + 1 < cap) {
        dst[pos++] = *src++;
    }
    dst[pos] = '\0';
    return pos;
}

int http_get(uint32_t ip, uint16_t port, const char *path, char *out, size_t cap) {
    tcp_conn_t conn;
    char req[256];
    size_t n = 0;
    size_t total = 0;
    int headers_done = 0;

    if (!out || cap == 0 || !path || path[0] != '/') {
        return -1;
    }
    if (!net_present()) {
        return -1;
    }

    out[0] = '\0';
    n = scopy(req, sizeof(req), "GET ");
    n = append(req, sizeof(req), n, path);
    n = append(req, sizeof(req), n, " HTTP/1.0\r\nConnection: close\r\n\r\n");

    if (tcp_connect(&conn, ip, port) != 0) {
        return -1;
    }
    if (tcp_send(&conn, req, n) != 0) {
        tcp_close(&conn);
        return -1;
    }

    for (int tries = 0; tries < 300000 && !conn.error; ++tries) {
        net_poll();

        if (conn.rx_len > 0) {
            size_t i = 0;
            while (i < conn.rx_len) {
                if (!headers_done) {
                    size_t j = i;
                    int found = 0;
                    while (j + 3 < conn.rx_len) {
                        if (conn.rx_buf[j] == '\r' && conn.rx_buf[j + 1] == '\n' &&
                            conn.rx_buf[j + 2] == '\r' && conn.rx_buf[j + 3] == '\n') {
                            headers_done = 1;
                            i = j + 4;
                            found = 1;
                            break;
                        }
                        j++;
                    }
                    if (!found) {
                        break;
                    }
                    continue;
                }

                if (total + 1 >= cap) {
                    tcp_close(&conn);
                    out[total] = '\0';
                    return (int)total;
                }
                out[total++] = conn.rx_buf[i++];
            }

            if (i > 0) {
                size_t rem = conn.rx_len - i;
                for (size_t k = 0; k < rem; ++k) {
                    conn.rx_buf[k] = conn.rx_buf[i + k];
                }
                conn.rx_len = rem;
            }
        }

        if (conn.closed) {
            break;
        }
        for (volatile int s = 0; s < 500; ++s) { }
    }

    tcp_close(&conn);
    if (total < cap) {
        out[total] = '\0';
    }
    return headers_done ? (int)total : -1;
}
