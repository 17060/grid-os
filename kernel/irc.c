#include "console.h"
#include "irc.h"
#include "net.h"
#include "tcp.h"

#include <stddef.h>
#include <stdint.h>

static size_t copy_str(char *dst, size_t cap, const char *src) {
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

static size_t build_line(char *out, size_t cap, const char *a, const char *b) {
    size_t i = 0;
    while (a && a[i] && i + 1 < cap) {
        out[i] = a[i];
        i++;
    }
    size_t j = 0;
    while (b && b[j] && i + 1 < cap) {
        out[i] = b[j];
        i++;
        j++;
    }
    out[i] = '\0';
    return i;
}

static void send_line(tcp_conn_t *c, const char *line) {
    char buf[256];
    size_t n = build_line(buf, sizeof(buf) - 2, line, "\r\n");
    tcp_send(c, buf, n);
}

static int starts_with(const char *s, const char *p) {
    while (*p) {
        if (*s++ != *p++) {
            return 0;
        }
    }
    return 1;
}

static void parse_privmsg(const char *line) {
    /* :nick!user@host PRIVMSG #chan :message */
    if (line[0] != ':') {
        return;
    }
    const char *nick_end = line + 1;
    while (*nick_end && *nick_end != '!' && *nick_end != ' ') {
        nick_end++;
    }
    if (*nick_end != '!') {
        return;
    }
    const char *cmd = nick_end + 1;
    while (*cmd && *cmd != ' ') {
        cmd++;
    }
    while (*cmd == ' ') {
        cmd++;
    }
    if (!starts_with(cmd, "PRIVMSG ")) {
        return;
    }
    const char *chan = cmd + 8;
    const char *msg = chan;
    while (*msg && *msg != ' ') {
        msg++;
    }
    while (*msg == ' ') {
        msg++;
    }
    if (*msg == ':') {
        msg++;
    }

    console_set_color(GRID_COL_OK);
    console_write("<");
    size_t nlen = 0;
    const char *np = line + 1;
    while (np + nlen < nick_end && nlen < 16) {
        console_write_char(np[nlen]);
        nlen++;
    }
    console_write("> ");
    console_set_color(GRID_COL_DEFAULT);
    console_write_line(msg);
}

static void handle_server_line(const char *line, tcp_conn_t *c, const char *channel,
                               int *registered) {
    if (line[0] == '\0') {
        return;
    }

    /* PING :server */
    if (starts_with(line, "PING ")) {
        char pong[128];
        copy_str(pong, sizeof(pong) - 6, "PONG ");
        copy_str(pong + 5, sizeof(pong) - 11, line + 5);
        send_line(c, pong);
        return;
    }

    /* Numeric 001 RPL_WELCOME marks successful registration. */
    if (!*registered && starts_with(line, ":") ) {
        const char *sp = line + 1;
        while (*sp && *sp != ' ') {
            sp++;
        }
        while (*sp == ' ') {
            sp++;
        }
        if (sp[0] == '0' && sp[1] == '0' && sp[2] == '1') {
            *registered = 1;
            console_set_color(GRID_COL_OK);
            console_write_line("IRC: registered with server.");
            console_set_color(GRID_COL_DEFAULT);
            char join[160];
            build_line(join, sizeof(join), "JOIN ", channel);
            send_line(c, join);
            console_write("IRC: joining ");
            console_write_line(channel);
        }
        return;
    }

    if (starts_with(line, "ERROR")) {
        console_set_color(GRID_COL_ERROR);
        console_write_line(line);
        console_set_color(GRID_COL_DEFAULT);
        c->closed = 1;
        return;
    }

    parse_privmsg(line);
}

int irc_session(const char *host_ip, uint16_t port, const char *nick, const char *channel,
                uint32_t duration_loops) {
    uint32_t ip;
    if (net_parse_ip(host_ip, &ip) != 0) {
        console_set_color(GRID_COL_ERROR);
        console_write_line("IRC: bad server IP.");
        console_set_color(GRID_COL_DEFAULT);
        return -1;
    }
    if (!net_present()) {
        console_set_color(GRID_COL_ERROR);
        console_write_line("IRC: no network device.");
        console_set_color(GRID_COL_DEFAULT);
        return -1;
    }

    tcp_conn_t c;
    console_write("IRC: connecting to ");
    console_write(host_ip);
    console_write_char(':');
    {
        char pb[8];
        size_t p = 0;
        uint16_t v = port;
        char tmp[8];
        int t = 0;
        if (v == 0) {
            tmp[t++] = '0';
        }
        while (v > 0) {
            tmp[t++] = (char)('0' + (v % 10));
            v /= 10;
        }
        while (t > 0) {
            pb[p++] = tmp[--t];
        }
        pb[p] = '\0';
        console_write(pb);
    }
    console_write_line(" ...");

    tcp_init();
    net_set_tcp_input(tcp_input);
    if (tcp_connect(&c, ip, port) != 0) {
        console_set_color(GRID_COL_ERROR);
        console_write_line("IRC: connection failed (no SYN-ACK).");
        console_set_color(GRID_COL_DEFAULT);
        return -1;
    }
    console_set_color(GRID_COL_OK);
    console_write_line("IRC: TCP connected.");
    console_set_color(GRID_COL_DEFAULT);

    /* Register: NICK + USER */
    char nick_line[80];
    build_line(nick_line, sizeof(nick_line), "NICK ", nick);
    send_line(&c, nick_line);

    char user_line[128];
    {
        size_t i = 0;
        const char *u = "USER ";
        while (u[i]) {
            user_line[i] = u[i];
            i++;
        }
        size_t j = 0;
        while (nick[j]) {
            user_line[i++] = nick[j++];
        }
        const char *rest = " 0 * :Grid OS";
        size_t k = 0;
        while (rest[k] && i + 1 < sizeof(user_line)) {
            user_line[i++] = rest[k++];
        }
        user_line[i] = '\0';
    }
    send_line(&c, user_line);

    int registered = 0;
    char line[512];
    size_t line_len = 0;

    for (uint32_t i = 0; i < duration_loops; ++i) {
        int n = tcp_recv(&c, 60);
        if (n < 0) {
            break;
        }
        if (c.closed || c.error) {
            break;
        }
        for (size_t b = 0; b < (size_t)n; ++b) {
            char ch = (char)c.rx_buf[b];
            if (ch == '\r') {
                continue;
            }
            if (ch == '\n') {
                line[line_len] = '\0';
                if (line_len > 0) {
                    handle_server_line(line, &c, channel, &registered);
                }
                line_len = 0;
            } else if (line_len + 1 < sizeof(line)) {
                line[line_len++] = ch;
            }
        }
        if ((i % 1000u) == 0u && i > 0) {
            /* keep the connection alive by polling; PING/PONG handled above */
        }
    }

    if (registered) {
        char quit[96];
        build_line(quit, sizeof(quit), "QUIT :Grid OS 5.1 — end of line", "");
        send_line(&c, quit);
    }
    tcp_close(&c);
    console_set_color(GRID_COL_DIM);
    console_write_line("IRC: session ended.");
    console_set_color(GRID_COL_DEFAULT);
    return registered ? 0 : -1;
}
