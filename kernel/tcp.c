#include "console.h"
#include "net.h"
#include "tcp.h"

#include <stddef.h>
#include <stdint.h>

#define TCP_WINDOW 4096u

static tcp_conn_t *conn_slots[TCP_MAX_CONNECTIONS];
static tcp_conn_t pending_slots[TCP_MAX_PENDING];
static int pending_used[TCP_MAX_PENDING];
static uint16_t listen_ports[TCP_MAX_LISTENERS];
static int listen_count = 0;
static uint16_t next_local_port = 0xC000u;

static void put_u16_be(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)(v & 0xFF);
}

static void put_u32_be(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)(v & 0xFF);
}

static uint16_t get_u16_be(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static uint32_t get_u32_be(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

static uint32_t ones_complement(const uint8_t *data, size_t len, uint32_t seed) {
    uint32_t sum = seed;
    for (size_t i = 0; i + 1 < len; i += 2) {
        sum += (uint32_t)(((uint16_t)data[i] << 8) | data[i + 1]);
    }
    if (len & 1) {
        sum += (uint32_t)((uint16_t)data[len - 1] << 8);
    }
    return sum;
}

static uint16_t finalize_checksum(uint32_t sum) {
    while (sum >> 16) {
        sum = (sum & 0xFFFFu) + (sum >> 16);
    }
    return (uint16_t)(~sum & 0xFFFFu);
}

static uint16_t tcp_checksum(uint32_t src_ip, uint32_t dst_ip,
                             const uint8_t *seg, size_t seg_len) {
    uint8_t pseudo[12];
    put_u32_be(pseudo + 0, src_ip);
    put_u32_be(pseudo + 4, dst_ip);
    pseudo[8] = 0;
    pseudo[9] = 6;
    put_u16_be(pseudo + 10, (uint16_t)seg_len);
    uint32_t sum = ones_complement(pseudo, 12, 0);
    sum = ones_complement(seg, seg_len, sum);
    return finalize_checksum(sum);
}

static void send_segment(tcp_conn_t *c, uint8_t flags, const uint8_t *data, size_t len) {
    uint8_t seg[1500];
    if (20 + len > sizeof(seg)) {
        len = sizeof(seg) - 20;
    }

    for (size_t i = 0; i < 20; ++i) {
        seg[i] = 0;
    }
    put_u16_be(seg + 0, c->local_port);
    put_u16_be(seg + 2, c->remote_port);
    put_u32_be(seg + 4, c->tx_seq);
    put_u32_be(seg + 8, c->rx_seq);
    seg[12] = 0x50;  /* data offset = 5 (20 bytes) */
    seg[13] = flags;
    put_u16_be(seg + 14, TCP_WINDOW);
    if (data && len) {
        for (size_t i = 0; i < len; ++i) {
            seg[20 + i] = data[i];
        }
    }
    uint16_t csum = tcp_checksum(net_local_ip(), c->remote_ip, seg, 20 + len);
    seg[16] = (uint8_t)(csum >> 8);
    seg[17] = (uint8_t)(csum & 0xFF);

    net_send_ip(c->remote_ip, 6, seg, 20 + len);
}

static int register_conn(tcp_conn_t *c) {
    for (int i = 0; i < TCP_MAX_CONNECTIONS; ++i) {
        if (conn_slots[i] == 0) {
            conn_slots[i] = c;
            return 0;
        }
    }
    return -1;
}

static void unregister_conn(tcp_conn_t *c) {
    for (int i = 0; i < TCP_MAX_CONNECTIONS; ++i) {
        if (conn_slots[i] == c) {
            conn_slots[i] = 0;
            return;
        }
    }
}

static tcp_conn_t *lookup_conn(uint32_t remote_ip, uint16_t remote_port, uint16_t local_port) {
    for (int i = 0; i < TCP_MAX_CONNECTIONS; ++i) {
        tcp_conn_t *c = conn_slots[i];
        if (!c) {
            continue;
        }
        if (c->local_port == local_port && c->remote_port == remote_port &&
            c->remote_ip == remote_ip) {
            return c;
        }
    }
    return 0;
}

static int listen_port_active(uint16_t port) {
    for (int i = 0; i < listen_count; ++i) {
        if (listen_ports[i] == port) {
            return 1;
        }
    }
    return 0;
}

static int alloc_pending(tcp_conn_t **out) {
    for (int i = 0; i < TCP_MAX_PENDING; ++i) {
        if (!pending_used[i]) {
            pending_used[i] = 1;
            for (size_t k = 0; k < sizeof(pending_slots[i]); ++k) {
                ((uint8_t *)&pending_slots[i])[k] = 0;
            }
            *out = &pending_slots[i];
            return 0;
        }
    }
    return -1;
}

static void free_pending(tcp_conn_t *c) {
    for (int i = 0; i < TCP_MAX_PENDING; ++i) {
        if (&pending_slots[i] == c) {
            pending_used[i] = 0;
            return;
        }
    }
}

static int accept_pending(tcp_conn_t *p) {
    if (!p || !p->established || p->closed) {
        return -1;
    }
    if (register_conn(p) != 0) {
        return -1;
    }
    for (int i = 0; i < TCP_MAX_PENDING; ++i) {
        if (&pending_slots[i] == p) {
            pending_used[i] = 0;
            return 0;
        }
    }
    return 0;
}

static void reject_syn(uint32_t src_ip, uint16_t src_port, uint16_t dst_port,
                       uint32_t seq, uint32_t ack) {
    tcp_conn_t tmp;
    for (size_t k = 0; k < sizeof(tmp); ++k) {
        ((uint8_t *)&tmp)[k] = 0;
    }
    tmp.remote_ip = src_ip;
    tmp.remote_port = src_port;
    tmp.local_port = dst_port;
    tmp.tx_seq = ack;
    tmp.rx_seq = seq + 1;
    tmp.established = 1;
    send_segment(&tmp, TCP_RST | TCP_ACK, 0, 0);
}

static uint32_t isn_seed = 0x47424C45u;

static uint32_t next_isn(void) {
    isn_seed = isn_seed * 1103515245u + 12345u;
    return isn_seed;
}

static int handle_listen_syn(uint32_t src_ip, const uint8_t *pkt, size_t len) {
    (void)len;
    uint16_t src_port = get_u16_be(pkt + 0);
    uint16_t dst_port = get_u16_be(pkt + 2);
    uint32_t seq = get_u32_be(pkt + 4);
    uint32_t ack = get_u32_be(pkt + 8);

    if (!listen_port_active(dst_port)) {
        return 0;
    }

    tcp_conn_t *c = 0;
    if (alloc_pending(&c) != 0) {
        reject_syn(src_ip, src_port, dst_port, seq, ack);
        return -1;
    }

    c->remote_ip = src_ip;
    c->remote_port = src_port;
    c->local_port = dst_port;
    c->tx_seq = next_isn();
    c->rx_seq = seq + 1;
    c->established = 1;
    c->closed = 0;
    c->error = 0;
    c->rx_len = 0;

    send_segment(c, TCP_SYN | TCP_ACK, 0, 0);
    c->tx_seq++;
    return 0;
}

static uint16_t alloc_local_port(void) {
    next_local_port++;
    if (next_local_port < 0xC001u) {
        next_local_port = 0xC001u;
    }
    return next_local_port;
}

void tcp_init(void) {
    for (int i = 0; i < TCP_MAX_CONNECTIONS; ++i) {
        conn_slots[i] = 0;
    }
    for (int i = 0; i < TCP_MAX_PENDING; ++i) {
        pending_used[i] = 0;
    }
    listen_count = 0;
    next_local_port = 0xC000u;
    net_set_tcp_input(tcp_input);
}

int tcp_listen(uint16_t port) {
    if (port == 0) {
        return -1;
    }
    if (listen_port_active(port)) {
        return 0;
    }
    if (listen_count >= TCP_MAX_LISTENERS) {
        return -1;
    }
    listen_ports[listen_count++] = port;
    return 0;
}

void tcp_unlisten(uint16_t port) {
    for (int i = 0; i < listen_count; ++i) {
        if (listen_ports[i] == port) {
            for (int j = i + 1; j < listen_count; ++j) {
                listen_ports[j - 1] = listen_ports[j];
            }
            listen_count--;
            return;
        }
    }
}

int tcp_listen_active(uint16_t port) {
    return listen_port_active(port);
}

int tcp_accept(tcp_conn_t **out) {
    if (!out) {
        return -1;
    }
    *out = 0;
    for (int i = 0; i < TCP_MAX_PENDING; ++i) {
        tcp_conn_t *p = &pending_slots[i];
        if (!pending_used[i] || !p->established || p->closed) {
            continue;
        }
        if (accept_pending(p) != 0) {
            tcp_close(p);
            free_pending(p);
            return -1;
        }
        *out = p;
        return 0;
    }
    return -1;
}

int tcp_accept_port(tcp_conn_t **out, uint16_t local_port) {
    if (!out || local_port == 0) {
        return -1;
    }
    *out = 0;
    for (int i = 0; i < TCP_MAX_PENDING; ++i) {
        tcp_conn_t *p = &pending_slots[i];
        if (!pending_used[i] || !p->established || p->closed) {
            continue;
        }
        if (p->local_port != local_port) {
            continue;
        }
        if (accept_pending(p) != 0) {
            tcp_close(p);
            free_pending(p);
            return -1;
        }
        *out = p;
        return 0;
    }
    return -1;
}

int tcp_connect(tcp_conn_t *c, uint32_t ip, uint16_t port) {
    if (!net_present() || !c) {
        return -1;
    }
    if (register_conn(c) != 0) {
        return -1;
    }

    c->remote_ip = ip;
    c->remote_port = port;
    c->local_port = alloc_local_port();
    c->tx_seq = next_isn();
    c->rx_seq = 0;
    c->established = 0;
    c->closed = 0;
    c->error = 0;
    c->rx_len = 0;

    send_segment(c, TCP_SYN, 0, 0);
    c->tx_seq++;

    for (int i = 0; i < 400; ++i) {
        net_poll();
        if (c->established) {
            return 0;
        }
        if (c->closed || c->error) {
            unregister_conn(c);
            return -1;
        }
        for (volatile int s = 0; s < 20000; ++s) { }
    }
    unregister_conn(c);
    return -1;
}

int tcp_send(tcp_conn_t *c, const void *data, size_t len) {
    if (!c || !c->established || c->closed) {
        return -1;
    }
    const uint8_t *p = (const uint8_t *)data;
    size_t off = 0;
    while (off < len) {
        size_t chunk = len - off;
        if (chunk > 1000) {
            chunk = 1000;
        }
        send_segment(c, TCP_PSH | TCP_ACK, p + off, chunk);
        c->tx_seq += (uint32_t)chunk;
        off += chunk;
        for (int s = 0; s < 50; ++s) {
            net_poll();
        }
    }
    return 0;
}

size_t tcp_peek(const tcp_conn_t *c, void *out, size_t cap) {
    if (!c || !out || cap == 0) {
        return 0;
    }
    size_t n = c->rx_len < cap ? c->rx_len : cap;
    for (size_t i = 0; i < n; ++i) {
        ((uint8_t *)out)[i] = c->rx_buf[i];
    }
    return n;
}

size_t tcp_consume(tcp_conn_t *c, size_t n) {
    if (!c || n == 0) {
        return 0;
    }
    if (n > c->rx_len) {
        n = c->rx_len;
    }
    if (n == 0) {
        return 0;
    }
    for (size_t i = n; i < c->rx_len; ++i) {
        c->rx_buf[i - n] = c->rx_buf[i];
    }
    c->rx_len -= n;
    return n;
}

int tcp_recv(tcp_conn_t *c, uint32_t timeout_loops) {
    if (!c || !c->established || c->closed) {
        return -1;
    }
    if (c->rx_len > 0) {
        return (int)c->rx_len;
    }
    for (uint32_t i = 0; i < timeout_loops; ++i) {
        net_poll();
        if (c->rx_len > 0 || c->closed || c->error) {
            return (int)c->rx_len;
        }
        for (volatile int s = 0; s < 20000; ++s) { }
    }
    return (int)c->rx_len;
}

void tcp_close(tcp_conn_t *c) {
    if (!c) {
        return;
    }
    if (c->established && !c->closed) {
        send_segment(c, TCP_FIN | TCP_ACK, 0, 0);
        c->tx_seq++;
        for (int i = 0; i < 100; ++i) {
            net_poll();
            if (c->closed) {
                break;
            }
            for (volatile int s = 0; s < 10000; ++s) { }
        }
    }
    unregister_conn(c);
}

void tcp_input(uint32_t src_ip, const uint8_t *pkt, size_t len) {
    if (len < 20) {
        return;
    }
    uint16_t src_port = get_u16_be(pkt + 0);
    uint16_t dst_port = get_u16_be(pkt + 2);
    uint8_t flags = pkt[13];
    tcp_conn_t *c = lookup_conn(src_ip, src_port, dst_port);
    if (!c) {
        if ((flags & TCP_SYN) && !(flags & TCP_ACK)) {
            (void)handle_listen_syn(src_ip, pkt, len);
        }
        return;
    }

    uint32_t seq = get_u32_be(pkt + 4);
    uint8_t data_off = (uint8_t)((pkt[12] >> 4) * 4);
    if (data_off < 20 || data_off > len) {
        return;
    }

    const uint8_t *data = pkt + data_off;
    size_t data_len = len - data_off;

    if (flags & TCP_RST) {
        c->closed = 1;
        c->error = 1;
        return;
    }

    if ((flags & TCP_SYN) && (flags & TCP_ACK)) {
        c->rx_seq = seq + 1;
        c->tx_seq = get_u32_be(pkt + 8);
        send_segment(c, TCP_ACK, 0, 0);
        c->established = 1;
        return;
    }

    if (flags & TCP_ACK) {
        if (data_len > 0) {
            if (c->rx_seq == 0) {
                c->rx_seq = seq;
            }
            if (seq == c->rx_seq) {
                size_t avail = sizeof(c->rx_buf) - c->rx_len;
                size_t copy = data_len < avail ? data_len : avail;
                for (size_t i = 0; i < copy; ++i) {
                    c->rx_buf[c->rx_len + i] = data[i];
                }
                c->rx_len += copy;
                c->rx_seq += (uint32_t)copy;
                if (copy < data_len) {
                    c->error = 1;
                }
                send_segment(c, TCP_ACK, 0, 0);
            }
        }
        if (flags & TCP_FIN) {
            c->rx_seq += 1;
            send_segment(c, TCP_ACK, 0, 0);
            send_segment(c, TCP_FIN | TCP_ACK, 0, 0);
            c->tx_seq++;
            c->closed = 1;
        }
    }
}
