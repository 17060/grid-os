#include "http.h"
#include "net.h"
#include "tcp.h"

#include <stddef.h>
#include <stdint.h>

static struct {
    int active;
    uint32_t ip;
    uint16_t port;
    tcp_conn_t conn;
} g_http_pool;

static size_t str_len(const char *s) {
    size_t n = 0;
    while (s[n]) {
        n++;
    }
    return n;
}

static int header_has_close(const uint8_t *buf, size_t len) {
    for (size_t i = 0; i + 11 < len; ++i) {
        if (buf[i] == 'C' || buf[i] == 'c') {
            if ((buf[i + 1] == 'o' || buf[i + 1] == 'O') &&
                (buf[i + 2] == 'n' || buf[i + 2] == 'N') &&
                (buf[i + 3] == 'n' || buf[i + 3] == 'N') &&
                (buf[i + 4] == 'e' || buf[i + 4] == 'E') &&
                (buf[i + 5] == 'c' || buf[i + 5] == 'C') &&
                (buf[i + 6] == 't' || buf[i + 6] == 'T') &&
                (buf[i + 7] == 'i' || buf[i + 7] == 'I') &&
                (buf[i + 8] == 'o' || buf[i + 8] == 'O') &&
                (buf[i + 9] == 'n' || buf[i + 9] == 'N') &&
                buf[i + 10] == ':' &&
                buf[i + 11] == ' ') {
                size_t j = i + 12;
                while (j < len && (buf[j] == ' ' || buf[j] == '\t')) {
                    j++;
                }
                if (j + 4 < len && buf[j] == 'c' && buf[j + 1] == 'l' &&
                    buf[j + 2] == 'o' && buf[j + 3] == 's' && buf[j + 4] == 'e') {
                    return 1;
                }
            }
        }
    }
    return 0;
}

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

void http_close_idle(void) {
    if (g_http_pool.active) {
        tcp_close(&g_http_pool.conn);
        g_http_pool.active = 0;
    }
}

int http_get(uint32_t ip, uint16_t port, const char *path, char *out, size_t cap) {
    tcp_conn_t *conn;
    char req[256];
    size_t n = 0;
    size_t total = 0;
    int headers_done = 0;
    size_t path_len;
    int saw_close = 0;

    if (!out || cap == 0 || !path || path[0] != '/') {
        return -1;
    }
    if (!net_present()) {
        return -1;
    }

    path_len = str_len(path);
    if (path_len + 48 >= sizeof(req)) {
        return -1;
    }

    out[0] = '\0';

    if (g_http_pool.active &&
        (g_http_pool.ip != ip || g_http_pool.port != port ||
         !g_http_pool.conn.established || g_http_pool.conn.closed ||
         g_http_pool.conn.error)) {
        http_close_idle();
    }

    if (!g_http_pool.active) {
        if (tcp_connect(&g_http_pool.conn, ip, port) != 0) {
            return -1;
        }
        g_http_pool.ip = ip;
        g_http_pool.port = port;
        g_http_pool.active = 1;
    }
    conn = &g_http_pool.conn;

    n = scopy(req, sizeof(req), "GET ");
    n = append(req, sizeof(req), n, path);
    if (n != 4 + path_len) {
        http_close_idle();
        return -1;
    }
    n = append(req, sizeof(req), n, " HTTP/1.1\r\nHost: grid\r\nConnection: keep-alive\r\n\r\n");
    if (n + 1 >= sizeof(req)) {
        http_close_idle();
        return -1;
    }

    if (tcp_send(conn, req, n) != 0) {
        http_close_idle();
        return -1;
    }

    for (int tries = 0; tries < 300000 && !conn->error; ++tries) {
        net_poll();

        if (conn->rx_len > 0) {
            size_t i = 0;
            while (i < conn->rx_len) {
                if (!headers_done) {
                    size_t j = i;
                    int found = 0;
                    while (j + 3 < conn->rx_len) {
                        if (conn->rx_buf[j] == '\r' && conn->rx_buf[j + 1] == '\n' &&
                            conn->rx_buf[j + 2] == '\r' && conn->rx_buf[j + 3] == '\n') {
                            if (header_has_close(conn->rx_buf, j + 4)) {
                                saw_close = 1;
                            }
                            headers_done = 1;
                            i = j + 4;
                            found = 1;
                            break;
                        }
                        j++;
                    }
                    if (!found) {
                        if (conn->rx_len >= sizeof(conn->rx_buf)) {
                            conn->error = 1;
                        }
                        break;
                    }
                    continue;
                }

                if (total + 1 >= cap) {
                    if (saw_close || conn->closed) {
                        http_close_idle();
                    } else {
                        conn->rx_len = 0;
                    }
                    out[total] = '\0';
                    return (int)total;
                }
                out[total++] = conn->rx_buf[i++];
            }

            if (i > 0) {
                size_t rem = conn->rx_len - i;
                for (size_t k = 0; k < rem; ++k) {
                    conn->rx_buf[k] = conn->rx_buf[i + k];
                }
                conn->rx_len = rem;
            }
        }

        if (conn->closed) {
            saw_close = 1;
            break;
        }
        for (volatile int s = 0; s < 500; ++s) { }
    }

    if (saw_close || conn->closed || conn->error) {
        http_close_idle();
    } else {
        conn->rx_len = 0;
    }

    if (total < cap) {
        out[total] = '\0';
    }
    return headers_done ? (int)total : -1;
}

int http_get_host(const char *host, uint16_t port, const char *path, char *out, size_t cap) {
    uint32_t ip;
    if (net_resolve_host(host, &ip) != 0) {
        return -1;
    }
    return http_get(ip, port, path, out, cap);
}
