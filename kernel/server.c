#include "server.h"

#include "net.h"
#include "tcp.h"

#include <stddef.h>
#include <stdint.h>

typedef struct {
    int used;
    tcp_conn_t *conn;
    char line_buf[GRID_SERVER_LINE_MAX];
    size_t line_len;
} grid_server_slot_t;

static grid_server_slot_t g_slots[GRID_SERVER_SLOTS];
static uint16_t g_listen_ports[GRID_SERVER_LISTEN_MAX];
static int g_listen_count = 0;

static int streq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

static int starts_with(const char *s, const char *pfx) {
    while (*pfx) {
        if (*s++ != *pfx++) {
            return 0;
        }
    }
    return 1;
}

static void copy_str(char *d, size_t cap, const char *s) {
    size_t n = 0;
    while (s[n] && n + 1 < cap) {
        d[n] = s[n];
        n++;
    }
    d[n] = '\0';
}

static size_t str_len(const char *s) {
    size_t n = 0;
    while (s[n]) {
        n++;
    }
    return n;
}

static void append_str(char *d, size_t cap, const char *s) {
    size_t n = str_len(d);
    while (*s && n + 1 < cap) {
        d[n++] = *s++;
    }
    d[n] = '\0';
}

static void append_char(char *d, size_t cap, char c) {
    size_t n = str_len(d);
    if (n + 1 < cap) {
        d[n++] = c;
        d[n] = '\0';
    }
}

static int slot_index(grid_server_slot_t *s) {
    return (int)(s - g_slots) + 1;
}

static grid_server_slot_t *find_slot(int slot) {
    if (slot < 1 || slot > GRID_SERVER_SLOTS) {
        return 0;
    }
    grid_server_slot_t *s = &g_slots[slot - 1];
    return s->used ? s : 0;
}

static grid_server_slot_t *alloc_slot(tcp_conn_t *conn) {
    for (int i = 0; i < GRID_SERVER_SLOTS; ++i) {
        if (!g_slots[i].used) {
            g_slots[i].used = 1;
            g_slots[i].conn = conn;
            g_slots[i].line_len = 0;
            g_slots[i].line_buf[0] = '\0';
            return &g_slots[i];
        }
    }
    return 0;
}

static int port_tracked(uint16_t port) {
    for (int i = 0; i < g_listen_count; ++i) {
        if (g_listen_ports[i] == port) {
            return 1;
        }
    }
    return 0;
}

static void drain_lines(grid_server_slot_t *s) {
    uint8_t tmp[128];
    if (!s || !s->conn) {
        return;
    }
    while (1) {
        size_t got = tcp_peek(s->conn, tmp, sizeof(tmp));
        if (got == 0) {
            break;
        }
        size_t consumed = 0;
        for (size_t i = 0; i < got; ++i) {
            char c = (char)tmp[i];
            consumed = i + 1;
            if (c == '\r') {
                continue;
            }
            if (c == '\n') {
                if (s->line_len < GRID_SERVER_LINE_MAX) {
                    s->line_buf[s->line_len] = '\0';
                }
                tcp_consume(s->conn, consumed);
                return;
            }
            if (s->line_len + 1 < GRID_SERVER_LINE_MAX) {
                s->line_buf[s->line_len++] = c;
            }
        }
        tcp_consume(s->conn, consumed);
    }
}

void grid_server_init(void) {
    for (int i = 0; i < GRID_SERVER_SLOTS; ++i) {
        g_slots[i].used = 0;
        g_slots[i].conn = 0;
        g_slots[i].line_len = 0;
        g_slots[i].line_buf[0] = '\0';
    }
    g_listen_count = 0;
}

int grid_server_listen(uint16_t port) {
    if (!net_present() || port == 0) {
        return -1;
    }
    if (port_tracked(port)) {
        return 0;
    }
    if (g_listen_count >= GRID_SERVER_LISTEN_MAX) {
        return -1;
    }
    if (tcp_listen(port) != 0) {
        return -1;
    }
    g_listen_ports[g_listen_count++] = port;
    return 0;
}

int grid_server_unlisten(uint16_t port) {
    tcp_unlisten(port);
    for (int i = 0; i < g_listen_count; ++i) {
        if (g_listen_ports[i] == port) {
            for (int j = i + 1; j < g_listen_count; ++j) {
                g_listen_ports[j - 1] = g_listen_ports[j];
            }
            g_listen_count--;
            break;
        }
    }
    return 0;
}

void grid_server_stop_all(void) {
    for (int i = 0; i < GRID_SERVER_SLOTS; ++i) {
        if (g_slots[i].used && g_slots[i].conn) {
            tcp_close(g_slots[i].conn);
            g_slots[i].used = 0;
            g_slots[i].conn = 0;
        }
    }
    while (g_listen_count > 0) {
        grid_server_unlisten(g_listen_ports[g_listen_count - 1]);
    }
}

int grid_server_listening(uint16_t port) {
    return port_tracked(port);
}

void grid_server_poll(void) {
    net_poll();
    for (int i = 0; i < GRID_SERVER_SLOTS; ++i) {
        if (!g_slots[i].used || !g_slots[i].conn) {
            continue;
        }
        if (g_slots[i].conn->closed || g_slots[i].conn->error) {
            tcp_close(g_slots[i].conn);
            g_slots[i].used = 0;
            g_slots[i].conn = 0;
            g_slots[i].line_len = 0;
            g_slots[i].line_buf[0] = '\0';
            continue;
        }
        drain_lines(&g_slots[i]);
    }
}

int grid_server_accept(void) {
    for (int li = 0; li < g_listen_count; ++li) {
        tcp_conn_t *conn = 0;
        if (tcp_accept_port(&conn, g_listen_ports[li]) != 0 || !conn) {
            continue;
        }
        grid_server_slot_t *slot = alloc_slot(conn);
        if (!slot) {
            tcp_close(conn);
            return 0;
        }
        grid_server_reply(slot_index(slot), "220 Flynn Grid server ready");
        grid_server_reply(slot_index(slot), "220 Type HELP for commands");
        return slot_index(slot);
    }
    return 0;
}

int grid_server_read_line(int slot, char *line, size_t cap) {
    grid_server_slot_t *s = find_slot(slot);
    if (!s || !line || cap == 0) {
        return -1;
    }
    drain_lines(s);
    if (s->line_len == 0) {
        line[0] = '\0';
        return 0;
    }
    copy_str(line, cap, s->line_buf);
    s->line_len = 0;
    s->line_buf[0] = '\0';
    return 1;
}

int grid_server_write(int slot, const char *text) {
    grid_server_slot_t *s = find_slot(slot);
    if (!s || !s->conn || !text) {
        return -1;
    }
    return tcp_send(s->conn, text, str_len(text));
}

int grid_server_reply(int slot, const char *text) {
    grid_server_slot_t *s = find_slot(slot);
    char buf[GRID_SERVER_LINE_MAX + 4];
    size_t n;
    if (!s || !s->conn || !text) {
        return -1;
    }
    n = 0;
    while (text[n] && n + 3 < sizeof(buf)) {
        buf[n] = text[n];
        n++;
    }
    buf[n++] = '\r';
    buf[n++] = '\n';
    return tcp_send(s->conn, buf, n);
}

void grid_server_close(int slot) {
    grid_server_slot_t *s = find_slot(slot);
    if (!s) {
        return;
    }
    if (s->conn) {
        tcp_close(s->conn);
    }
    s->used = 0;
    s->conn = 0;
    s->line_len = 0;
    s->line_buf[0] = '\0';
}

int grid_server_dispatch_builtin(int slot, const char *line) {
    if (!line) {
        return 0;
    }
    if (streq(line, "PING")) {
        grid_server_reply(slot, "PONG");
        return 1;
    }
    if (streq(line, "HELP")) {
        grid_server_reply(slot, "Commands: PING HELP STATUS ECHO <text> QUIT");
        grid_server_reply(slot, "GridBASIC: use SUB handlers for custom keywords");
        return 1;
    }
    if (streq(line, "STATUS")) {
        char st[160];
        grid_server_format_status(st, sizeof(st));
        grid_server_reply(slot, st);
        return 1;
    }
    if (streq(line, "QUIT") || streq(line, "EXIT")) {
        grid_server_reply(slot, "221 End of line");
        grid_server_close(slot);
        return 1;
    }
    if (starts_with(line, "ECHO ")) {
        grid_server_reply(slot, line + 5);
        return 1;
    }
    return 0;
}

void grid_server_format_status(char *out, size_t cap) {
    int clients = 0;
    if (!out || cap == 0) {
        return;
    }
    out[0] = '\0';
    for (int i = 0; i < GRID_SERVER_SLOTS; ++i) {
        if (g_slots[i].used) {
            clients++;
        }
    }
    append_str(out, cap, "Grid server clients=");
    char num[8];
    int v = clients;
    int k = 0;
    if (v == 0) {
        num[k++] = '0';
    } else {
        char t[8];
        int tlen = 0;
        while (v > 0) {
            t[tlen++] = (char)('0' + (v % 10));
            v /= 10;
        }
        while (tlen > 0) {
            num[k++] = t[--tlen];
        }
    }
    num[k] = '\0';
    append_str(out, cap, num);
    append_str(out, cap, " listen=");
    if (g_listen_count == 0) {
        append_str(out, cap, "none");
        return;
    }
    for (int i = 0; i < g_listen_count; ++i) {
        if (i > 0) {
            append_char(out, cap, ',');
        }
        v = g_listen_ports[i];
        k = 0;
        if (v == 0) {
            num[k++] = '0';
        } else {
            char t[8];
            int tlen = 0;
            while (v > 0) {
                t[tlen++] = (char)('0' + (v % 10));
                v /= 10;
            }
            while (tlen > 0) {
                num[k++] = t[--tlen];
            }
        }
        num[k] = '\0';
        append_str(out, cap, num);
    }
}
