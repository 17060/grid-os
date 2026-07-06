#include "console.h"
#include "irc.h"
#include "net.h"
#include "tcp.h"

#include <stddef.h>
#include <stdint.h>

#define IRC_MAX_CHANNELS 8
#define IRC_QUEUE_SIZE   16
#define IRC_NICK_MAX     32
#define IRC_HOST_MAX     40
#define IRC_CHAN_MAX     64

typedef struct {
    int active;
    int registered;
    tcp_conn_t conn;
    char host[IRC_HOST_MAX];
    uint32_t host_ip;
    uint16_t port;
    char nick[IRC_NICK_MAX];
    char channels[IRC_MAX_CHANNELS][IRC_CHAN_MAX];
    int n_channels;
    char queue[IRC_QUEUE_SIZE][IRC_LINE_MAX];
    int queue_head;
    int queue_count;
    char parse_buf[IRC_LINE_MAX];
    size_t parse_len;
    int echo_console;
    int line_overflow;
} irc_state_t;

static irc_state_t g_irc;

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
    char buf[IRC_LINE_MAX];
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

static void fmt_ip(uint32_t ip, char *buf, size_t cap) {
    size_t n = 0;
    for (int o = 0; o < 4; ++o) {
        uint32_t b = (ip >> (24 - o * 8)) & 0xFFu;
        if (o > 0 && n + 1 < cap) {
            buf[n++] = '.';
        }
        char tmp[4];
        int t = 0;
        if (b == 0) {
            tmp[t++] = '0';
        } else {
            while (b > 0) {
                tmp[t++] = (char)('0' + (b % 10u));
                b /= 10u;
            }
        }
        while (t > 0 && n + 1 < cap) {
            buf[n++] = tmp[--t];
        }
    }
    buf[n] = '\0';
}

static void enqueue_line(const char *line) {
    if (!line || !line[0]) {
        return;
    }
    if (g_irc.queue_count >= IRC_QUEUE_SIZE) {
        g_irc.queue_head = (g_irc.queue_head + 1) % IRC_QUEUE_SIZE;
        g_irc.queue_count--;
    }
    int slot = (g_irc.queue_head + g_irc.queue_count) % IRC_QUEUE_SIZE;
    copy_str(g_irc.queue[slot], IRC_LINE_MAX, line);
    g_irc.queue_count++;
}

static int channel_known(const char *chan) {
    for (int i = 0; i < g_irc.n_channels; ++i) {
        const char *a = g_irc.channels[i];
        const char *b = chan;
        int same = 1;
        while (*a || *b) {
            if (*a != *b) {
                same = 0;
                break;
            }
            if (*a) {
                a++;
            }
            if (*b) {
                b++;
            }
        }
        if (same) {
            return 1;
        }
    }
    return 0;
}

static int remember_channel(const char *chan) {
    if (!chan || !chan[0] || g_irc.n_channels >= IRC_MAX_CHANNELS) {
        return -1;
    }
    if (channel_known(chan)) {
        return 0;
    }
    copy_str(g_irc.channels[g_irc.n_channels], IRC_CHAN_MAX, chan);
    g_irc.n_channels++;
    return 0;
}

static void forget_channel(const char *chan) {
    for (int i = 0; i < g_irc.n_channels; ++i) {
        const char *a = g_irc.channels[i];
        const char *b = chan;
        int same = 1;
        while (*a || *b) {
            if (*a != *b) {
                same = 0;
                break;
            }
            if (*a) {
                a++;
            }
            if (*b) {
                b++;
            }
        }
        if (same) {
            for (int j = i + 1; j < g_irc.n_channels; ++j) {
                copy_str(g_irc.channels[j - 1], IRC_CHAN_MAX, g_irc.channels[j]);
            }
            g_irc.n_channels--;
            return;
        }
    }
}

static void console_privmsg(const char *line) {
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
    const char *msg = cmd + 8;
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

static void format_chat_line(const char *line, char *out, size_t cap) {
    if (line[0] != ':') {
        copy_str(out, cap, line);
        return;
    }
    const char *nick_end = line + 1;
    while (*nick_end && *nick_end != '!' && *nick_end != ' ') {
        nick_end++;
    }
    const char *cmd = nick_end + 1;
    while (*cmd && *cmd != ' ') {
        cmd++;
    }
    while (*cmd == ' ') {
        cmd++;
    }

    size_t n = 0;
    out[n++] = '<';
    const char *np = line + 1;
    while (np < nick_end && n + 1 < cap) {
        out[n++] = *np++;
    }
    if (n + 2 < cap) {
        out[n++] = '>';
        out[n++] = ' ';
    }

    if (starts_with(cmd, "PRIVMSG ") || starts_with(cmd, "NOTICE ")) {
        const char *target = cmd + (starts_with(cmd, "NOTICE ") ? 7 : 8);
        while (*target && *target != ' ') {
            if (n + 1 < cap) {
                out[n++] = *target;
            }
            target++;
        }
        if (n + 2 < cap) {
            out[n++] = ':';
            out[n++] = ' ';
        }
        while (*target == ' ') {
            target++;
        }
        if (*target == ':') {
            target++;
        }
        while (*target && n + 1 < cap) {
            out[n++] = *target++;
        }
    } else if (starts_with(cmd, "JOIN ") || starts_with(cmd, "PART ") ||
               starts_with(cmd, "NICK ") || starts_with(cmd, "TOPIC ")) {
        while (*cmd && n + 1 < cap) {
            out[n++] = *cmd++;
        }
    } else if (cmd[0] >= '0' && cmd[0] <= '9') {
        while (*cmd && n + 1 < cap) {
            out[n++] = *cmd++;
        }
    } else {
        while (*cmd && n + 1 < cap) {
            out[n++] = *cmd++;
        }
    }
    out[n] = '\0';
}

static void handle_server_line(const char *line) {
    if (line[0] == '\0') {
        return;
    }

    if (starts_with(line, "PING ")) {
        char pong[128];
        copy_str(pong, sizeof(pong) - 6, "PONG ");
        copy_str(pong + 5, sizeof(pong) - 11, line + 5);
        send_line(&g_irc.conn, pong);
        return;
    }

    if (!g_irc.registered && line[0] == ':') {
        const char *sp = line + 1;
        while (*sp && *sp != ' ') {
            sp++;
        }
        while (*sp == ' ') {
            sp++;
        }
        if (sp[0] == '0' && sp[1] == '0' && sp[2] == '1') {
            g_irc.registered = 1;
            enqueue_line("IRC: registered");
            if (g_irc.echo_console) {
                console_set_color(GRID_COL_OK);
                console_write_line("IRC: registered with server.");
                console_set_color(GRID_COL_DEFAULT);
            }
        }
    }

    if (starts_with(line, "ERROR")) {
        char msg[IRC_LINE_MAX];
        build_line(msg, sizeof(msg), "ERROR: ", line);
        enqueue_line(msg);
        if (g_irc.echo_console) {
            console_set_color(GRID_COL_ERROR);
            console_write_line(line);
            console_set_color(GRID_COL_DEFAULT);
        }
        g_irc.conn.closed = 1;
        return;
    }


    char formatted[IRC_LINE_MAX];
    format_chat_line(line, formatted, sizeof(formatted));
    if (formatted[0]) {
        enqueue_line(formatted);
    }

    if (g_irc.echo_console && line[0] == ':') {
        const char *cmd = line + 1;
        while (*cmd && *cmd != ' ') {
            cmd++;
        }
        while (*cmd == ' ') {
            cmd++;
        }
        if (starts_with(cmd, "PRIVMSG ")) {
            console_privmsg(line);
        }
    }
}

static void process_rx_bytes(const uint8_t *data, size_t n) {
    for (size_t b = 0; b < n; ++b) {
        char ch = (char)data[b];
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            g_irc.parse_buf[g_irc.parse_len] = '\0';
            if (g_irc.parse_len > 0) {
                handle_server_line(g_irc.parse_buf);
            }
            g_irc.parse_len = 0;
        } else if (g_irc.parse_len + 1 < IRC_LINE_MAX) {
            g_irc.parse_buf[g_irc.parse_len++] = ch;
        } else if (!g_irc.line_overflow) {
            g_irc.line_overflow = 1;
            enqueue_line("IRC: line truncated");
        }
    }
}

static void send_user(void) {
    char user_line[128];
    size_t i = 0;
    const char *u = "USER ";
    while (u[i]) {
        user_line[i] = u[i];
        i++;
    }
    size_t j = 0;
    while (g_irc.nick[j]) {
        user_line[i++] = g_irc.nick[j++];
    }
    const char *rest = " 0 * :Grid OS";
    size_t k = 0;
    while (rest[k] && i + 1 < sizeof(user_line)) {
        user_line[i++] = rest[k++];
    }
    user_line[i] = '\0';
    send_line(&g_irc.conn, user_line);
}

static void teardown(void) {
    if (g_irc.active && g_irc.conn.established && !g_irc.conn.closed) {
        tcp_close(&g_irc.conn);
    }
    g_irc.active = 0;
    g_irc.registered = 0;
    g_irc.n_channels = 0;
    g_irc.parse_len = 0;
    g_irc.queue_head = 0;
    g_irc.queue_count = 0;
    g_irc.echo_console = 0;
    g_irc.line_overflow = 0;
}

int irc_is_connected(void) {
    return g_irc.active && g_irc.conn.established && !g_irc.conn.closed && !g_irc.conn.error;
}

int irc_connect(const char *host_ip, uint16_t port, const char *nick) {
    if (!host_ip || !nick || !nick[0]) {
        return -1;
    }
    if (g_irc.active) {
        irc_disconnect();
    }
    if (net_resolve_host(host_ip, &g_irc.host_ip) != 0) {
        return -1;
    }
    if (!net_present()) {
        return -1;
    }

    copy_str(g_irc.host, IRC_HOST_MAX, host_ip);
    g_irc.port = port;
    copy_str(g_irc.nick, IRC_NICK_MAX, nick);
    g_irc.n_channels = 0;
    g_irc.queue_head = 0;
    g_irc.queue_count = 0;
    g_irc.parse_len = 0;
    g_irc.registered = 0;
    g_irc.echo_console = 0;
    g_irc.line_overflow = 0;

    if (tcp_connect(&g_irc.conn, g_irc.host_ip, port) != 0) {
        return -1;
    }

    g_irc.active = 1;

    char nick_line[80];
    build_line(nick_line, sizeof(nick_line), "NICK ", nick);
    send_line(&g_irc.conn, nick_line);
    send_user();
    enqueue_line("IRC: connected");
    return 0;
}

void irc_disconnect(void) {
    if (!g_irc.active) {
        return;
    }
    teardown();
}

int irc_join(const char *channel) {
    if (!irc_is_connected() || !channel || !channel[0]) {
        return -1;
    }
    char join[160];
    build_line(join, sizeof(join), "JOIN ", channel);
    send_line(&g_irc.conn, join);
    remember_channel(channel);
    return 0;
}

int irc_part(const char *channel) {
    if (!irc_is_connected() || !channel || !channel[0]) {
        return -1;
    }
    char part[160];
    build_line(part, sizeof(part), "PART ", channel);
    send_line(&g_irc.conn, part);
    forget_channel(channel);
    return 0;
}

int irc_say(const char *target, const char *msg) {
    if (!irc_is_connected() || !target || !target[0] || !msg) {
        return -1;
    }
    char line[IRC_LINE_MAX];
    size_t n = 0;
    const char *p = "PRIVMSG ";
    while (*p && n + 1 < sizeof(line)) {
        line[n++] = *p++;
    }
    while (*target && n + 1 < sizeof(line)) {
        line[n++] = *target++;
    }
    if (n + 2 < sizeof(line)) {
        line[n++] = ' ';
        line[n++] = ':';
    }
    while (*msg && n + 1 < sizeof(line)) {
        line[n++] = *msg++;
    }
    line[n] = '\0';
    send_line(&g_irc.conn, line);
    return 0;
}

int irc_nick(const char *nick) {
    if (!irc_is_connected() || !nick || !nick[0]) {
        return -1;
    }
    char line[96];
    build_line(line, sizeof(line), "NICK ", nick);
    send_line(&g_irc.conn, line);
    copy_str(g_irc.nick, IRC_NICK_MAX, nick);
    return 0;
}

int irc_quit(const char *reason) {
    if (!irc_is_connected()) {
        return -1;
    }
    char quit[IRC_LINE_MAX];
    if (reason && reason[0]) {
        build_line(quit, sizeof(quit), "QUIT :", reason);
    } else {
        copy_str(quit, sizeof(quit), "QUIT :Grid OS");
    }
    send_line(&g_irc.conn, quit);
    teardown();
    return 0;
}

void irc_poll(void) {
    if (!g_irc.active) {
        return;
    }
    if (g_irc.conn.closed || g_irc.conn.error) {
        enqueue_line("IRC: disconnected");
        g_irc.active = 0;
        g_irc.registered = 0;
        return;
    }

    int n = tcp_recv(&g_irc.conn, 2);
    if (n > 0) {
        process_rx_bytes(g_irc.conn.rx_buf, (size_t)n);
    }
    if (g_irc.conn.closed || g_irc.conn.error) {
        enqueue_line("IRC: disconnected");
        g_irc.active = 0;
        g_irc.registered = 0;
    }
}

size_t irc_read(char *buf, size_t cap) {
    if (!buf || cap == 0) {
        return 0;
    }
    if (g_irc.queue_count == 0) {
        buf[0] = '\0';
        return 0;
    }
    size_t len = copy_str(buf, cap, g_irc.queue[g_irc.queue_head]);
    g_irc.queue_head = (g_irc.queue_head + 1) % IRC_QUEUE_SIZE;
    g_irc.queue_count--;
    return len;
}

void irc_status(char *buf, size_t cap) {
    if (!buf || cap == 0) {
        return;
    }
    if (!irc_is_connected()) {
        copy_str(buf, cap, "disconnected");
        return;
    }
    char ipbuf[20];
    fmt_ip(g_irc.host_ip, ipbuf, sizeof(ipbuf));
    char out[IRC_LINE_MAX];
    size_t n = 0;
    const char *p = "connected ";
    while (*p && n + 1 < cap) {
        out[n++] = *p++;
    }
    p = ipbuf;
    while (*p && n + 1 < cap) {
        out[n++] = *p++;
    }
    if (n + 1 < cap) {
        out[n++] = ':';
    }
    uint16_t v = g_irc.port;
    char pb[8];
    size_t pp = 0;
    char tmp[8];
    int t = 0;
    if (v == 0) {
        tmp[t++] = '0';
    }
    while (v > 0) {
        tmp[t++] = (char)('0' + (v % 10));
        v = (uint16_t)(v / 10);
    }
    while (t > 0) {
        pb[pp++] = tmp[--t];
    }
    pb[pp] = '\0';
    p = pb;
    while (*p && n + 1 < cap) {
        out[n++] = *p++;
    }
    if (n + 1 < cap) {
        out[n++] = ' ';
    }
    p = "nick=";
    while (*p && n + 1 < cap) {
        out[n++] = *p++;
    }
    p = g_irc.nick;
    while (*p && n + 1 < cap) {
        out[n++] = *p++;
    }
    if (g_irc.registered && n + 12 < cap) {
        out[n++] = ' ';
        p = "registered";
        while (*p && n + 1 < cap) {
            out[n++] = *p++;
        }
    }
    out[n] = '\0';
    copy_str(buf, cap, out);
}

int irc_session(const char *host_ip, uint16_t port, const char *nick, const char *channel,
                uint32_t duration_loops) {
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
            v = (uint16_t)(v / 10);
        }
        while (t > 0) {
            pb[p++] = tmp[--t];
        }
        pb[p] = '\0';
        console_write(pb);
    }
    console_write_line(" ...");

    if (irc_connect(host_ip, port, nick) != 0) {
        console_set_color(GRID_COL_ERROR);
        console_write_line("IRC: connection failed.");
        console_set_color(GRID_COL_DEFAULT);
        return -1;
    }

    g_irc.echo_console = 1;
    console_set_color(GRID_COL_OK);
    console_write_line("IRC: TCP connected.");
    console_set_color(GRID_COL_DEFAULT);

    int joined = 0;
    for (uint32_t i = 0; i < duration_loops; ++i) {
        irc_poll();
        if (!irc_is_connected()) {
            break;
        }
        if (g_irc.registered && !joined && channel && channel[0]) {
            irc_join(channel);
            console_write("IRC: joining ");
            console_write_line(channel);
            joined = 1;
        }
        for (volatile uint32_t s = 0; s < 20000u; ++s) {
        }
    }

    int was_registered = g_irc.registered;
    if (irc_is_connected()) {
        irc_quit("Grid OS — end of line");
    } else {
        irc_disconnect();
    }

    console_set_color(GRID_COL_DIM);
    console_write_line("IRC: session ended.");
    console_set_color(GRID_COL_DEFAULT);
    return was_registered ? 0 : -1;
}
