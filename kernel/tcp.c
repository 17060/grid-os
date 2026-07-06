#include "console.h"
#include "net.h"
#include "tcp.h"

#include <stddef.h>
#include <stdint.h>

#define TCP_WINDOW 4096u

static tcp_conn_t *active_conn;

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
    /* checksum at 16-17, written below */
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

void tcp_init(void) {
    active_conn = 0;
    net_set_tcp_input(tcp_input);
}

static uint32_t isn_seed = 0x47424C45u;

static uint32_t next_isn(void) {
    isn_seed = isn_seed * 1103515245u + 12345u;
    return isn_seed;
}

int tcp_connect(tcp_conn_t *c, uint32_t ip, uint16_t port) {
    if (!net_present()) {
        return -1;
    }
    c->remote_ip = ip;
    c->remote_port = port;
    c->local_port = 0xC001u;
    c->tx_seq = next_isn();
    c->rx_seq = 0;
    c->established = 0;
    c->closed = 0;
    c->error = 0;
    c->rx_len = 0;

    active_conn = c;

    /* SYN */
    send_segment(c, TCP_SYN, 0, 0);
    c->tx_seq++;

    /* Wait for SYN-ACK */
    for (int i = 0; i < 400; ++i) {
        net_poll();
        if (c->established) {
            return 0;
        }
        if (c->closed || c->error) {
            active_conn = 0;
            return -1;
        }
        for (volatile int s = 0; s < 20000; ++s) { }
    }
    active_conn = 0;
    return -1;
}

int tcp_send(tcp_conn_t *c, const void *data, size_t len) {
    if (!c->established || c->closed) {
        return -1;
    }
    active_conn = c;
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
        /* drain ACKs so the device keeps moving */
        for (int s = 0; s < 50; ++s) {
            net_poll();
        }
    }
    return 0;
}

int tcp_recv(tcp_conn_t *c, uint32_t timeout_loops) {
    if (!c->established || c->closed) {
        return -1;
    }
    active_conn = c;
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
    if (!c->established) {
        active_conn = 0;
        return;
    }
    active_conn = c;
    send_segment(c, TCP_FIN | TCP_ACK, 0, 0);
    c->tx_seq++;
    for (int i = 0; i < 100; ++i) {
        net_poll();
        if (c->closed) {
            break;
        }
        for (volatile int s = 0; s < 10000; ++s) { }
    }
    active_conn = 0;
}

void tcp_input(uint32_t src_ip, const uint8_t *pkt, size_t len) {
    (void)src_ip;
    tcp_conn_t *c = active_conn;
    if (!c || len < 20) {
        return;
    }
    uint16_t src_port = get_u16_be(pkt + 0);
    uint16_t dst_port = get_u16_be(pkt + 2);
    uint32_t seq = get_u32_be(pkt + 4);
    uint32_t ack = get_u32_be(pkt + 8);
    uint8_t data_off = (uint8_t)((pkt[12] >> 4) * 4);
    uint8_t flags = pkt[13];
    if (data_off < 20 || data_off > len) {
        return;
    }
    if (dst_port != c->local_port || src_port != c->remote_port) {
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
        /* SYN-ACK: confirm our SYN was acked (ack == isn+1) */
        c->rx_seq = seq + 1;
        c->tx_seq = ack;  /* server tells us our next seq */
        /* send ACK */
        send_segment(c, TCP_ACK, 0, 0);
        c->established = 1;
        return;
    }

    if (flags & TCP_ACK) {
        if (data_len > 0) {
            /* deliver */
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
