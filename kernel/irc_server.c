#include "irc_server.h"

#include "log.h"
#include "net.h"
#include "tcp.h"

#include <stddef.h>
#include <stdint.h>

#define IRC_SERVER_NAME "grid.flynn"
#define IRC_SERVER_MOTD "Welcome to Flynn Grid IRC — use !help for bot commands"

typedef struct {
    int used;
    tcp_conn_t *conn;
    char nick[GRID_IRC_SERVER_NICK_MAX];
    char user[GRID_IRC_SERVER_NICK_MAX];
    int has_nick;
    int has_user;
    int registered;
    char channels[4][GRID_IRC_SERVER_CHAN_MAX];
    int n_channels;
    char line_buf[GRID_IRC_SERVER_LINE_MAX];
    size_t line_len;
} irc_server_client_t;

typedef struct {
    char kind[16];
    int slot;
    char nick[GRID_IRC_SERVER_NICK_MAX];
    char target[GRID_IRC_SERVER_CHAN_MAX];
    char text[GRID_IRC_SERVER_LINE_MAX];
} irc_server_event_t;

static irc_server_client_t g_clients[GRID_IRC_SERVER_SLOTS];
static uint16_t g_listen_ports[GRID_IRC_SERVER_LISTEN_MAX];
static int g_listen_count = 0;
static irc_server_event_t g_events[GRID_IRC_SERVER_EVENTS];
static int g_event_head = 0;
static int g_event_count = 0;
static irc_server_event_t g_last_event;
static char g_cmd_prefix[8] = "!";

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
    if (!d || cap == 0) {
        return;
    }
    if (s) {
        while (s[n] && n + 1 < cap) {
            d[n] = s[n];
            n++;
        }
    }
    d[n] = '\0';
}

static size_t str_len(const char *s) {
    size_t n = 0;
    while (s && s[n]) {
        n++;
    }
    return n;
}

static void append_str(char *d, size_t cap, const char *s) {
    size_t n = str_len(d);
    while (s && *s && n + 1 < cap) {
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

static int slot_index(irc_server_client_t *c) {
    return (int)(c - g_clients) + 1;
}

static irc_server_client_t *find_client(int slot) {
    if (slot < 1 || slot > GRID_IRC_SERVER_SLOTS) {
        return 0;
    }
    irc_server_client_t *c = &g_clients[slot - 1];
    return c->used ? c : 0;
}

static irc_server_client_t *alloc_client(tcp_conn_t *conn) {
    for (int i = 0; i < GRID_IRC_SERVER_SLOTS; ++i) {
        if (!g_clients[i].used) {
            g_clients[i].used = 1;
            g_clients[i].conn = conn;
            g_clients[i].nick[0] = '\0';
            g_clients[i].user[0] = '\0';
            g_clients[i].has_nick = 0;
            g_clients[i].has_user = 0;
            g_clients[i].registered = 0;
            g_clients[i].n_channels = 0;
            g_clients[i].line_len = 0;
            g_clients[i].line_buf[0] = '\0';
            return &g_clients[i];
        }
    }
    return 0;
}

static void client_prefix(irc_server_client_t *c, char *out, size_t cap) {
    if (!c || !out || cap == 0) {
        return;
    }
    out[0] = '\0';
    append_str(out, cap, c->nick);
    append_str(out, cap, "!");
    append_str(out, cap, c->user[0] ? c->user : "grid");
    append_str(out, cap, "@");
    append_str(out, cap, IRC_SERVER_NAME);
}

static void send_raw(irc_server_client_t *c, const char *line) {
    char buf[GRID_IRC_SERVER_LINE_MAX + 4];
    size_t n;
    if (!c || !c->conn || !line) {
        return;
    }
    n = 0;
    while (line[n] && n + 3 < sizeof(buf)) {
        buf[n] = line[n];
        n++;
    }
    buf[n++] = '\r';
    buf[n++] = '\n';
    tcp_send(c->conn, buf, n);
}

static void send_numeric(irc_server_client_t *c, int code, const char *text) {
    char line[GRID_IRC_SERVER_LINE_MAX];
    char num[8];
    int v = code;
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
    copy_str(line, sizeof(line), ":");
    append_str(line, sizeof(line), IRC_SERVER_NAME);
    append_char(line, sizeof(line), ' ');
    append_str(line, sizeof(line), num);
    append_char(line, sizeof(line), ' ');
    append_str(line, sizeof(line), c->nick[0] ? c->nick : "*");
    if (text && text[0]) {
        append_char(line, sizeof(line), ' ');
        append_str(line, sizeof(line), text);
    }
    send_raw(c, line);
}

static void enqueue_event(const char *kind, int slot, const char *nick,
                            const char *target, const char *text) {
    if (g_event_count >= GRID_IRC_SERVER_EVENTS) {
        log_event("IRC server event queue overflow");
        g_event_head = (g_event_head + 1) % GRID_IRC_SERVER_EVENTS;
        g_event_count--;
    }
    int idx = (g_event_head + g_event_count) % GRID_IRC_SERVER_EVENTS;
    copy_str(g_events[idx].kind, sizeof(g_events[idx].kind), kind);
    g_events[idx].slot = slot;
    copy_str(g_events[idx].nick, sizeof(g_events[idx].nick), nick ? nick : "");
    copy_str(g_events[idx].target, sizeof(g_events[idx].target), target ? target : "");
    copy_str(g_events[idx].text, sizeof(g_events[idx].text), text ? text : "");
    g_event_count++;
}

static int client_in_channel(irc_server_client_t *c, const char *chan) {
    if (!c || !chan) {
        return 0;
    }
    for (int i = 0; i < c->n_channels; ++i) {
        if (streq(c->channels[i], chan)) {
            return 1;
        }
    }
    return 0;
}

static void client_join_channel(irc_server_client_t *c, const char *chan) {
    if (!c || !chan || !chan[0] || c->n_channels >= 4) {
        return;
    }
    if (client_in_channel(c, chan)) {
        return;
    }
    copy_str(c->channels[c->n_channels], GRID_IRC_SERVER_CHAN_MAX, chan);
    c->n_channels++;
}

static void client_part_channel(irc_server_client_t *c, const char *chan) {
    if (!c || !chan) {
        return;
    }
    for (int i = 0; i < c->n_channels; ++i) {
        if (streq(c->channels[i], chan)) {
            for (int j = i + 1; j < c->n_channels; ++j) {
                copy_str(c->channels[j - 1], GRID_IRC_SERVER_CHAN_MAX, c->channels[j]);
            }
            c->n_channels--;
            return;
        }
    }
}

static void broadcast_channel(const char *chan, const char *line, irc_server_client_t *skip) {
    for (int i = 0; i < GRID_IRC_SERVER_SLOTS; ++i) {
        irc_server_client_t *c = &g_clients[i];
        if (!c->used || !c->conn || c->conn->closed || c == skip) {
            continue;
        }
        if (client_in_channel(c, chan)) {
            send_raw(c, line);
        }
    }
}

static void send_join(irc_server_client_t *c, const char *chan) {
    char line[GRID_IRC_SERVER_LINE_MAX];
    char prefix[80];
    client_prefix(c, prefix, sizeof(prefix));
    copy_str(line, sizeof(line), ":");
    append_str(line, sizeof(line), prefix);
    append_str(line, sizeof(line), " JOIN ");
    append_str(line, sizeof(line), chan);
    broadcast_channel(chan, line, 0);
}

static void send_names(irc_server_client_t *c, const char *chan) {
    char names[GRID_IRC_SERVER_LINE_MAX];
    char line[GRID_IRC_SERVER_LINE_MAX];
    int first = 1;
    copy_str(names, sizeof(names), "");
    for (int i = 0; i < GRID_IRC_SERVER_SLOTS; ++i) {
        irc_server_client_t *m = &g_clients[i];
        if (!m->used || !client_in_channel(m, chan) || !m->nick[0]) {
            continue;
        }
        if (!first) {
            append_char(names, sizeof(names), ' ');
        }
        append_str(names, sizeof(names), m->nick);
        first = 0;
    }
    copy_str(line, sizeof(line), "=");
    append_char(line, sizeof(line), ' ');
    append_str(line, sizeof(line), chan);
    append_char(line, sizeof(line), ' ');
    append_str(line, sizeof(line), ":");
    append_str(line, sizeof(line), names[0] ? names : c->nick);
    send_numeric(c, 353, line);
    copy_str(line, sizeof(line), chan);
    append_str(line, sizeof(line), " :End of /NAMES list");
    send_numeric(c, 366, line);
}

static void maybe_register(irc_server_client_t *c) {
    char motd[96];
    if (!c || c->registered || !c->has_nick || !c->has_user) {
        return;
    }
    c->registered = 1;
    send_numeric(c, 1, ":Welcome to Flynn Grid IRC");
    send_numeric(c, 2, ":Your host is " IRC_SERVER_NAME);
    send_numeric(c, 3, ":This server was created for Grid OS");
    copy_str(motd, sizeof(motd), ":- ");
    append_str(motd, sizeof(motd), IRC_SERVER_MOTD);
    send_numeric(c, 372, motd);
    send_numeric(c, 376, ":End of /MOTD command");
}

static void close_client(irc_server_client_t *c) {
    if (!c) {
        return;
    }
    if (c->conn) {
        tcp_close(c->conn);
    }
    c->used = 0;
    c->conn = 0;
    c->registered = 0;
    c->n_channels = 0;
    c->line_len = 0;
}

static void skip_spaces(const char **p) {
    while (**p == ' ') {
        (*p)++;
    }
}

static void next_token(const char **p, char *out, size_t cap) {
    size_t n = 0;
    skip_spaces(p);
    while (**p && **p != ' ' && n + 1 < cap) {
        out[n++] = *(*p)++;
    }
    out[n] = '\0';
}

static void rest_param(const char **p, char *out, size_t cap) {
    size_t n = 0;
    skip_spaces(p);
    if (**p == ':') {
        (*p)++;
    }
    while (**p && n + 1 < cap) {
        out[n++] = *(*p)++;
    }
    out[n] = '\0';
}

static int is_bot_command(const char *text) {
    if (!text || !text[0]) {
        return 0;
    }
    if (g_cmd_prefix[0] && starts_with(text, g_cmd_prefix)) {
        return 1;
    }
    return 0;
}

static int require_registered(irc_server_client_t *c) {
    if (c && c->registered) {
        return 1;
    }
    if (c) {
        send_numeric(c, 451, ":You have not registered");
    }
    return 0;
}

static void handle_privmsg(irc_server_client_t *c, const char *target, const char *text) {
    char line[GRID_IRC_SERVER_LINE_MAX];
    char prefix[80];
    int slot;
    if (!c || !target || !text) {
        return;
    }
    slot = slot_index(c);
    if (target[0] == '#') {
        if (is_bot_command(text)) {
            enqueue_event("PRIVMSG", slot, c->nick, target, text);
            return;
        }
        client_prefix(c, prefix, sizeof(prefix));
        copy_str(line, sizeof(line), ":");
        append_str(line, sizeof(line), prefix);
        append_str(line, sizeof(line), " PRIVMSG ");
        append_str(line, sizeof(line), target);
        append_str(line, sizeof(line), " :");
        append_str(line, sizeof(line), text);
        broadcast_channel(target, line, c);
        send_raw(c, line);
        return;
    }
    enqueue_event("PRIVMSG", slot, c->nick, target, text);
}

static void handle_line(irc_server_client_t *c, const char *line) {
    const char *p = line;
    char prefix[80];
    char cmd[32];
    char arg1[GRID_IRC_SERVER_CHAN_MAX];
    char arg2[GRID_IRC_SERVER_LINE_MAX];
    char pong[128];
    char part_msg[GRID_IRC_SERVER_LINE_MAX];
    char prefix_line[GRID_IRC_SERVER_LINE_MAX];
    int slot;

    if (!c || !line || !line[0]) {
        return;
    }

    prefix[0] = '\0';
    if (*p == ':') {
        p++;
        next_token(&p, prefix, sizeof(prefix));
    }
    next_token(&p, cmd, sizeof(cmd));
    if (!cmd[0]) {
        return;
    }

    slot = slot_index(c);

    if (streq(cmd, "NICK")) {
        rest_param(&p, arg1, sizeof(arg1));
        if (!arg1[0]) {
            return;
        }
        copy_str(c->nick, sizeof(c->nick), arg1);
        c->has_nick = 1;
        maybe_register(c);
        return;
    }
    if (streq(cmd, "USER")) {
        next_token(&p, arg1, sizeof(arg1));
        copy_str(c->user, sizeof(c->user), arg1);
        c->has_user = 1;
        maybe_register(c);
        return;
    }
    if (streq(cmd, "PASS")) {
        return;
    }
    if (streq(cmd, "PING")) {
        rest_param(&p, arg1, sizeof(arg1));
        copy_str(pong, sizeof(pong), ":");
        append_str(pong, sizeof(pong), IRC_SERVER_NAME);
        append_str(pong, sizeof(pong), " PONG ");
        append_str(pong, sizeof(pong), IRC_SERVER_NAME);
        append_char(pong, sizeof(pong), ' ');
        append_str(pong, sizeof(pong), arg1);
        send_raw(c, pong);
        return;
    }
    if (streq(cmd, "JOIN")) {
        if (!require_registered(c)) {
            return;
        }
        rest_param(&p, arg1, sizeof(arg1));
        if (!arg1[0]) {
            return;
        }
        if (arg1[0] != '#') {
            copy_str(arg2, sizeof(arg2), "#");
            append_str(arg2, sizeof(arg2), arg1);
            copy_str(arg1, sizeof(arg1), arg2);
        }
        client_join_channel(c, arg1);
        send_join(c, arg1);
        send_names(c, arg1);
        enqueue_event("JOIN", slot, c->nick, arg1, "");
        return;
    }
    if (streq(cmd, "PART")) {
        if (!require_registered(c)) {
            return;
        }
        next_token(&p, arg1, sizeof(arg1));
        rest_param(&p, arg2, sizeof(arg2));
        if (!arg1[0]) {
            return;
        }
        client_prefix(c, prefix_line, sizeof(prefix_line));
        copy_str(part_msg, sizeof(part_msg), ":");
        append_str(part_msg, sizeof(part_msg), prefix_line);
        append_str(part_msg, sizeof(part_msg), " PART ");
        append_str(part_msg, sizeof(part_msg), arg1);
        if (arg2[0]) {
            append_str(part_msg, sizeof(part_msg), " :");
            append_str(part_msg, sizeof(part_msg), arg2);
        }
        broadcast_channel(arg1, part_msg, 0);
        client_part_channel(c, arg1);
        enqueue_event("PART", slot, c->nick, arg1, arg2);
        return;
    }
    if (streq(cmd, "PRIVMSG")) {
        if (!require_registered(c)) {
            return;
        }
        next_token(&p, arg1, sizeof(arg1));
        rest_param(&p, arg2, sizeof(arg2));
        handle_privmsg(c, arg1, arg2);
        return;
    }
    if (streq(cmd, "QUIT")) {
        rest_param(&p, arg1, sizeof(arg1));
        client_prefix(c, prefix_line, sizeof(prefix_line));
        copy_str(part_msg, sizeof(part_msg), ":");
        append_str(part_msg, sizeof(part_msg), prefix_line);
        append_str(part_msg, sizeof(part_msg), " QUIT");
        if (arg1[0]) {
            append_str(part_msg, sizeof(part_msg), " :");
            append_str(part_msg, sizeof(part_msg), arg1);
        }
        for (int ci = 0; ci < c->n_channels; ++ci) {
            broadcast_channel(c->channels[ci], part_msg, 0);
        }
        enqueue_event("QUIT", slot, c->nick, "", arg1);
        close_client(c);
        return;
    }
    if (streq(cmd, "MODE") || streq(cmd, "WHO") || streq(cmd, "LIST")) {
        send_numeric(c, 321, ":Channel :Users Name");
        send_numeric(c, 323, ":End of /LIST");
        return;
    }
}

static void drain_lines(irc_server_client_t *c) {
    uint8_t tmp[128];
    if (!c || !c->conn) {
        return;
    }
    while (1) {
        size_t got = tcp_peek(c->conn, tmp, sizeof(tmp));
        if (got == 0) {
            break;
        }
        size_t consumed = 0;
        for (size_t i = 0; i < got; ++i) {
            char ch = (char)tmp[i];
            consumed = i + 1;
            if (ch == '\r') {
                continue;
            }
            if (ch == '\n') {
                if (c->line_len < GRID_IRC_SERVER_LINE_MAX) {
                    c->line_buf[c->line_len] = '\0';
                }
                handle_line(c, c->line_buf);
                c->line_len = 0;
                c->line_buf[0] = '\0';
                tcp_consume(c->conn, consumed);
                return;
            }
            if (c->line_len + 1 < GRID_IRC_SERVER_LINE_MAX) {
                c->line_buf[c->line_len++] = ch;
            }
        }
        tcp_consume(c->conn, consumed);
    }
}

void grid_irc_server_init(void) {
    for (int i = 0; i < GRID_IRC_SERVER_SLOTS; ++i) {
        g_clients[i].used = 0;
        g_clients[i].conn = 0;
    }
    g_listen_count = 0;
    g_event_head = 0;
    g_event_count = 0;
    g_last_event.kind[0] = '\0';
    copy_str(g_cmd_prefix, sizeof(g_cmd_prefix), "!");
}

int grid_irc_server_listen(uint16_t port) {
    if (!net_present() || port == 0) {
        return -1;
    }
    if (grid_irc_server_listening(port)) {
        return 0;
    }
    if (g_listen_count >= GRID_IRC_SERVER_LISTEN_MAX) {
        return -1;
    }
    if (tcp_listen(port) != 0) {
        return -1;
    }
    g_listen_ports[g_listen_count++] = port;
    return 0;
}

int grid_irc_server_unlisten(uint16_t port) {
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

void grid_irc_server_stop_all(void) {
    for (int i = 0; i < GRID_IRC_SERVER_SLOTS; ++i) {
        if (g_clients[i].used) {
            close_client(&g_clients[i]);
        }
    }
    while (g_listen_count > 0) {
        grid_irc_server_unlisten(g_listen_ports[g_listen_count - 1]);
    }
    g_event_head = 0;
    g_event_count = 0;
}

int grid_irc_server_listening(uint16_t port) {
    for (int i = 0; i < g_listen_count; ++i) {
        if (g_listen_ports[i] == port) {
            return 1;
        }
    }
    return 0;
}

void grid_irc_server_poll(void) {
    net_poll();
    for (int li = 0; li < g_listen_count; ++li) {
        tcp_conn_t *conn = 0;
        while (tcp_accept_port(&conn, g_listen_ports[li]) == 0 && conn) {
            irc_server_client_t *c = alloc_client(conn);
            if (!c) {
                tcp_close(conn);
                break;
            }
        }
    }
    for (int i = 0; i < GRID_IRC_SERVER_SLOTS; ++i) {
        irc_server_client_t *c = &g_clients[i];
        if (!c->used || !c->conn) {
            continue;
        }
        if (c->conn->closed || c->conn->error) {
            close_client(c);
            continue;
        }
        drain_lines(c);
    }
}

int grid_irc_server_event(char *out, size_t cap) {
    if (g_event_count == 0 || !out || cap == 0) {
        if (out && cap > 0) {
            out[0] = '\0';
        }
        return 0;
    }
    irc_server_event_t *ev = &g_events[g_event_head];
    g_last_event = *ev;
    out[0] = '\0';
    append_str(out, cap, ev->kind);
    append_char(out, cap, '|');
    char num[8];
    int v = ev->slot;
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
    append_char(out, cap, '|');
    append_str(out, cap, ev->nick);
    append_char(out, cap, '|');
    append_str(out, cap, ev->target);
    append_char(out, cap, '|');
    append_str(out, cap, ev->text);
    g_event_head = (g_event_head + 1) % GRID_IRC_SERVER_EVENTS;
    g_event_count--;
    return 1;
}

const char *grid_irc_server_event_kind(void) {
    return g_last_event.kind[0] ? g_last_event.kind : "";
}

int grid_irc_server_event_slot(void) {
    return g_last_event.slot;
}

void grid_irc_server_event_nick(char *out, size_t cap) {
    copy_str(out, cap, g_last_event.nick);
}

void grid_irc_server_event_target(char *out, size_t cap) {
    copy_str(out, cap, g_last_event.target);
}

void grid_irc_server_event_text(char *out, size_t cap) {
    copy_str(out, cap, g_last_event.text);
}

int grid_irc_server_say(int slot, const char *target, const char *text) {
    irc_server_client_t *c = find_client(slot);
    char line[GRID_IRC_SERVER_LINE_MAX];
    char prefix[80];
    if (!c || !c->conn || !target || !text) {
        return -1;
    }
    client_prefix(c, prefix, sizeof(prefix));
    copy_str(line, sizeof(line), ":");
    append_str(line, sizeof(line), prefix);
    append_str(line, sizeof(line), " PRIVMSG ");
    append_str(line, sizeof(line), target);
    append_str(line, sizeof(line), " :");
    append_str(line, sizeof(line), text);
    send_raw(c, line);
    if (target[0] == '#') {
        broadcast_channel(target, line, c);
    }
    return 0;
}

int grid_irc_server_notice(int slot, const char *target, const char *text) {
    irc_server_client_t *c = find_client(slot);
    char line[GRID_IRC_SERVER_LINE_MAX];
    char prefix[80];
    if (!c || !c->conn || !target || !text) {
        return -1;
    }
    client_prefix(c, prefix, sizeof(prefix));
    copy_str(line, sizeof(line), ":");
    append_str(line, sizeof(line), prefix);
    append_str(line, sizeof(line), " NOTICE ");
    append_str(line, sizeof(line), target);
    append_str(line, sizeof(line), " :");
    append_str(line, sizeof(line), text);
    send_raw(c, line);
    if (target[0] == '#') {
        broadcast_channel(target, line, c);
    }
    return 0;
}

static void send_server_line(const char *cmd, const char *target, const char *text) {
    char line[GRID_IRC_SERVER_LINE_MAX];
    if (!target || !text) {
        return;
    }
    copy_str(line, sizeof(line), ":");
    append_str(line, sizeof(line), IRC_SERVER_NAME);
    append_char(line, sizeof(line), ' ');
    append_str(line, sizeof(line), cmd);
    append_char(line, sizeof(line), ' ');
    append_str(line, sizeof(line), target);
    append_str(line, sizeof(line), " :");
    append_str(line, sizeof(line), text);
    if (target[0] == '#') {
        broadcast_channel(target, line, 0);
        return;
    }
    for (int i = 0; i < GRID_IRC_SERVER_SLOTS; ++i) {
        irc_server_client_t *c = &g_clients[i];
        if (!c->used || !c->conn || c->conn->closed) {
            continue;
        }
        if (streq(c->nick, target)) {
            send_raw(c, line);
            return;
        }
    }
}

int grid_irc_server_bot_say(const char *target, const char *text) {
    send_server_line("PRIVMSG", target, text);
    return 0;
}

int grid_irc_server_bot_notice(const char *target, const char *text) {
    send_server_line("NOTICE", target, text);
    return 0;
}

void grid_irc_server_client_nick(int slot, char *out, size_t cap) {
    irc_server_client_t *c = find_client(slot);
    if (!c) {
        if (out && cap > 0) {
            out[0] = '\0';
        }
        return;
    }
    copy_str(out, cap, c->nick);
}

void grid_irc_server_format_status(char *out, size_t cap) {
    int clients = 0;
    if (!out || cap == 0) {
        return;
    }
    out[0] = '\0';
    for (int i = 0; i < GRID_IRC_SERVER_SLOTS; ++i) {
        if (g_clients[i].used) {
            clients++;
        }
    }
    append_str(out, cap, "Flynn IRC server clients=");
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
