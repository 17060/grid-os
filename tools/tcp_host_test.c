/* Host-side tests for multi-connection TCP dispatch. */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include "tcp.h"
#include "net.h"

static void (*g_tcp_input)(uint32_t src_ip, const uint8_t *pkt, size_t len);
static uint32_t g_local_ip = 0x0A00020Fu;
static uint32_t g_syn_seq = 1000;

void net_set_tcp_input(void (*fn)(uint32_t src_ip, const uint8_t *pkt, size_t len)) {
    g_tcp_input = fn;
}

uint32_t net_local_ip(void) {
    return g_local_ip;
}

int net_present(void) {
    return 1;
}

void net_poll(void) {
    (void)0;
}

static uint16_t get_u16_be(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

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

static uint32_t get_u32_be(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

int net_send_ip(uint32_t ip, uint8_t proto, const uint8_t *data, size_t len) {
    const uint8_t *seg;
    uint8_t reply[20];

    if (proto != 6 || !data || len < 20 || !g_tcp_input) {
        return 0;
    }
    seg = (const uint8_t *)data;
    if ((seg[13] & TCP_SYN) && !(seg[13] & TCP_ACK)) {
        for (size_t i = 0; i < sizeof(reply); ++i) {
            reply[i] = 0;
        }
        put_u16_be(reply + 0, get_u16_be(seg + 2));
        put_u16_be(reply + 2, get_u16_be(seg + 0));
        put_u32_be(reply + 4, g_syn_seq);
        put_u32_be(reply + 8, get_u32_be(seg + 4) + 1u);
        reply[12] = 0x50;
        reply[13] = TCP_SYN | TCP_ACK;
        g_syn_seq += 1000;
        g_tcp_input(ip, reply, sizeof(reply));
    }
    return 0;
}

static void deliver_data(tcp_conn_t *c, const char *payload) {
    uint8_t pkt[256];
    size_t plen = strlen(payload);
    size_t total = 20 + plen;
    for (size_t i = 0; i < total; ++i) {
        pkt[i] = 0;
    }
    put_u16_be(pkt + 0, c->remote_port);
    put_u16_be(pkt + 2, c->local_port);
    put_u32_be(pkt + 4, c->rx_seq ? c->rx_seq : 1);
    pkt[12] = 0x50;
    pkt[13] = TCP_ACK | TCP_PSH;
    memcpy(pkt + 20, payload, plen);
    if (g_tcp_input) {
        g_tcp_input(c->remote_ip, pkt, total);
    }
}

static int test_dual_ports(void) {
    tcp_conn_t a;
    tcp_conn_t b;

    g_syn_seq = 1000;
    tcp_init();
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    a.remote_ip = 0x01020304u;
    a.remote_port = 80;
    b.remote_ip = 0x05060708u;
    b.remote_port = 443;

    if (tcp_connect(&a, a.remote_ip, a.remote_port) != 0 || !a.established) {
        return -1;
    }
    if (tcp_connect(&b, b.remote_ip, b.remote_port) != 0 || !b.established) {
        return -1;
    }
    if (a.local_port == b.local_port) {
        return -1;
    }

    deliver_data(&a, "AAA");
    deliver_data(&b, "BBB");
    if (a.rx_len != 3 || memcmp(a.rx_buf, "AAA", 3) != 0) {
        return -1;
    }
    if (b.rx_len != 3 || memcmp(b.rx_buf, "BBB", 3) != 0) {
        return -1;
    }

    tcp_close(&a);
    tcp_close(&b);
    return 0;
}

static int test_slot_limit(void) {
    tcp_conn_t conns[5];
    int i;

    g_syn_seq = 5000;
    tcp_init();
    for (i = 0; i < TCP_MAX_CONNECTIONS; ++i) {
        memset(&conns[i], 0, sizeof(conns[i]));
        conns[i].remote_ip = 0x0A000001u + (uint32_t)i;
        conns[i].remote_port = (uint16_t)(9000 + i);
        if (tcp_connect(&conns[i], conns[i].remote_ip, conns[i].remote_port) != 0) {
            return -1;
        }
    }

    memset(&conns[4], 0, sizeof(conns[4]));
    conns[4].remote_ip = 0x0A000099u;
    conns[4].remote_port = 9999;
    if (tcp_connect(&conns[4], conns[4].remote_ip, conns[4].remote_port) == 0) {
        return -1;
    }

    for (i = 0; i < TCP_MAX_CONNECTIONS; ++i) {
        tcp_close(&conns[i]);
    }
    return 0;
}

int main(void) {
    if (test_dual_ports() != 0) {
        fprintf(stderr, "tcp dual-port test failed\n");
        return 1;
    }
    if (test_slot_limit() != 0) {
        fprintf(stderr, "tcp slot-limit test failed\n");
        return 1;
    }
    printf("tcp host tests OK\n");
    return 0;
}
