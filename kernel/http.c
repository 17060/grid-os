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

static int parse_content_length(const uint8_t *buf, size_t len, size_t *out) {
    for (size_t i = 0; i + 15 < len; ++i) {
        if ((buf[i] == 'C' || buf[i] == 'c') &&
            (buf[i + 1] == 'o' || buf[i + 1] == 'O') &&
            (buf[i + 2] == 'n' || buf[i + 2] == 'N') &&
            (buf[i + 3] == 't' || buf[i + 3] == 'T') &&
            (buf[i + 4] == 'e' || buf[i + 4] == 'E') &&
            (buf[i + 5] == 'n' || buf[i + 5] == 'N') &&
            (buf[i + 6] == 't' || buf[i + 6] == 'T') &&
            buf[i + 7] == '-' &&
            (buf[i + 8] == 'L' || buf[i + 8] == 'l') &&
            (buf[i + 9] == 'e' || buf[i + 9] == 'E') &&
            (buf[i + 10] == 'n' || buf[i + 10] == 'N') &&
            (buf[i + 11] == 'g' || buf[i + 11] == 'G') &&
            (buf[i + 12] == 't' || buf[i + 12] == 'T') &&
            (buf[i + 13] == 'h' || buf[i + 13] == 'H') &&
            buf[i + 14] == ':') {
            size_t j = i + 15;
            while (j < len && (buf[j] == ' ' || buf[j] == '\t')) {
                j++;
            }
            size_t val = 0;
            int got = 0;
            while (j < len && buf[j] >= '0' && buf[j] <= '9') {
                val = val * 10u + (size_t)(buf[j] - '0');
                got = 1;
                j++;
            }
            if (got) {
                *out = val;
                return 0;
            }
        }
    }
    return -1;
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

static void format_ip(uint32_t ip, char *buf, size_t cap) {
    char tmp[16];
    size_t pos = 0;

    for (int part = 3; part >= 0; --part) {
        uint32_t octet = (ip >> (part * 8)) & 0xFFu;
        if (octet >= 100) {
            tmp[pos++] = (char)('0' + (octet / 100));
            octet %= 100;
            tmp[pos++] = (char)('0' + (octet / 10));
            tmp[pos++] = (char)('0' + (octet % 10));
        } else if (octet >= 10) {
            tmp[pos++] = (char)('0' + (octet / 10));
            tmp[pos++] = (char)('0' + (octet % 10));
        } else {
            tmp[pos++] = (char)('0' + octet);
        }
        if (part > 0) {
            tmp[pos++] = '.';
        }
    }
    tmp[pos] = '\0';
    scopy(buf, cap, tmp);
}

void http_close_idle(void) {
    if (g_http_pool.active) {
        tcp_close(&g_http_pool.conn);
        g_http_pool.active = 0;
    }
}

static int http_request(const char *method, const char *host, uint32_t ip, uint16_t port,
                        const char *path, const char *body, char *out, size_t cap) {
    tcp_conn_t *conn;
    char req[512];
    char hostbuf[48];
    size_t n = 0;
    size_t total = 0;
    size_t body_copied = 0;
    size_t content_length = 0;
    int headers_done = 0;
    int has_content_length = 0;
    size_t path_len;
    size_t body_len = 0;
    int saw_close = 0;
    int body_complete = 0;

    if (!out || cap == 0 || !path || path[0] != '/' || !method) {
        return -1;
    }
    if (!net_present()) {
        return -1;
    }
    if (body) {
        body_len = str_len(body);
    }

    path_len = str_len(path);
    if (path_len + 96 + body_len >= sizeof(req)) {
        return -1;
    }

    if (!host || !host[0]) {
        format_ip(ip, hostbuf, sizeof(hostbuf));
        host = hostbuf;
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

    n = scopy(req, sizeof(req), method);
    n = append(req, sizeof(req), n, " ");
    n = append(req, sizeof(req), n, path);
    n = append(req, sizeof(req), n, " HTTP/1.1\r\nHost: ");
    n = append(req, sizeof(req), n, host);
    n = append(req, sizeof(req), n, "\r\nConnection: keep-alive\r\n");
    if (body_len > 0) {
        char clen[16];
        size_t cl = 0;
        size_t tmp = body_len;
        char digits[12];
        int nd = 0;
        do {
            digits[nd++] = (char)('0' + (tmp % 10));
            tmp /= 10;
        } while (tmp > 0);
        while (nd > 0) {
            clen[cl++] = digits[--nd];
        }
        clen[cl] = '\0';
        n = append(req, sizeof(req), n, "Content-Length: ");
        n = append(req, sizeof(req), n, clen);
        n = append(req, sizeof(req), n, "\r\nContent-Type: text/plain\r\n");
    }
    n = append(req, sizeof(req), n, "\r\n");
    if (body_len > 0) {
        n = append(req, sizeof(req), n, body);
    }
    if (n + 1 >= sizeof(req)) {
        http_close_idle();
        return -1;
    }

    if (tcp_send(conn, req, n) != 0) {
        http_close_idle();
        return -1;
    }

    for (int tries = 0; tries < 300000 && !conn->error && !body_complete; ++tries) {
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
                            {
                                size_t cl = 0;
                                if (parse_content_length(conn->rx_buf, j + 4, &cl) == 0) {
                                    content_length = cl;
                                    has_content_length = 1;
                                }
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

                if (has_content_length && body_copied >= content_length) {
                    body_complete = 1;
                    break;
                }

                if (total + 1 >= cap) {
                    if (saw_close || conn->closed) {
                        http_close_idle();
                    } else if (has_content_length && body_copied >= content_length) {
                        body_complete = 1;
                    } else {
                        conn->rx_len = 0;
                    }
                    out[total] = '\0';
                    return (int)total;
                }
                out[total++] = conn->rx_buf[i++];
                body_copied++;
            }

            if (i > 0) {
                size_t rem = conn->rx_len - i;
                for (size_t k = 0; k < rem; ++k) {
                    conn->rx_buf[k] = conn->rx_buf[i + k];
                }
                conn->rx_len = rem;
            }
        }

        if (has_content_length && body_copied >= content_length) {
            body_complete = 1;
            break;
        }

        if (conn->closed) {
            saw_close = 1;
            break;
        }
        for (volatile int s = 0; s < 500; ++s) { }
    }

    if (saw_close || conn->closed || conn->error) {
        http_close_idle();
    } else if (body_complete) {
        /* Leave surplus bytes in rx_buf for the next keep-alive response. */
    } else {
        conn->rx_len = 0;
    }

    if (total < cap) {
        out[total] = '\0';
    }
    return headers_done ? (int)total : -1;
}

int http_get(uint32_t ip, uint16_t port, const char *path, char *out, size_t cap) {
    return http_request("GET", 0, ip, port, path, 0, out, cap);
}

int http_post(uint32_t ip, uint16_t port, const char *path, const char *body,
              char *out, size_t cap) {
    return http_request("POST", 0, ip, port, path, body ? body : "", out, cap);
}

int http_get_host(const char *host, uint16_t port, const char *path, char *out, size_t cap) {
    uint32_t ip;
    if (net_resolve_host(host, &ip) != 0) {
        return -1;
    }
    return http_request("GET", host, ip, port, path, 0, out, cap);
}

int http_post_host(const char *host, uint16_t port, const char *path, const char *body,
                   char *out, size_t cap) {
    uint32_t ip;
    if (net_resolve_host(host, &ip) != 0) {
        return -1;
    }
    return http_request("POST", host, ip, port, path, body, out, cap);
}
