#include "console.h"
#include "dns.h"
#include "memory.h"
#include "net.h"
#include "pci.h"

#include <stddef.h>
#include <stdint.h>

#define VIRTIO_VENDOR_ID        0x1AF4u
#define VIRTIO_DEV_NET_LEGACY   0x1000u

#define VIRTIO_STATUS_ACK         1u
#define VIRTIO_STATUS_DRIVER      2u
#define VIRTIO_STATUS_DRIVER_OK   4u
#define VIRTIO_STATUS_FEATURES_OK 8u

#define VRING_DESC_F_NEXT  1u
#define VRING_DESC_F_WRITE 2u

#define NET_RING_SIZE   8u
#define NET_HDR_SIZE    10u
#define NET_BUF_SIZE    1514u
#define NET_QUEUE_RX    0u
#define NET_QUEUE_TX    1u

#define ETH_TYPE_ARP 0x0806u
#define ETH_TYPE_IPV4 0x0800u

#define ICMP_ECHO_REPLY 0u
#define ICMP_ECHO_REQUEST 8u
#define IP_PROTO_ICMP 1u
#define IP_PROTO_TCP  6u
#define IP_PROTO_UDP  17u

#define NET_LOCAL_IP   0x0A00020Fu  /* 10.0.2.15 */
#define NET_GATEWAY_IP 0x0A000202u  /* 10.0.2.2  */

typedef struct {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} vring_desc_t;

typedef struct {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[NET_RING_SIZE];
} vring_avail_t;

typedef struct {
    uint32_t id;
    uint32_t len;
} vring_used_elem_t;

typedef struct {
    uint16_t flags;
    uint16_t idx;
    vring_used_elem_t ring[NET_RING_SIZE];
} vring_used_t;

typedef struct {
    int present;
    pci_device_t pci;
    uint16_t io_base;
    uint8_t mac[6];
    vring_desc_t *rx_desc;
    vring_avail_t *rx_avail;
    vring_used_t *rx_used;
    vring_desc_t *tx_desc;
    vring_avail_t *tx_avail;
    vring_used_t *tx_used;
    uint8_t *rx_buf[NET_RING_SIZE];
    uint8_t *tx_buf;
    uint16_t rx_last_used;
    uint16_t tx_last_used;
    uint16_t rx_avail_idx;
    uint32_t rx_pkts;
    uint32_t tx_pkts;
    uint32_t arp_replies;
    uint32_t icmp_replies;
    uint32_t ping_sent;
    uint32_t ping_replies;
    uint32_t ping_target_ip;
    uint32_t ping_got_reply;
    uint8_t gateway_mac[6];
    int gateway_resolved;
    void (*tcp_input_fn)(uint32_t src_ip, const uint8_t *pkt, size_t len);
} virtio_net_t;

static virtio_net_t net;

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline void outl(uint16_t port, uint32_t value) {
    __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t value;
    __asm__ volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static uint32_t host_features(void) {
    return inl((uint16_t)(net.io_base + 0u));
}

static void guest_features(uint32_t value) {
    outl((uint16_t)(net.io_base + 4u), value);
}

static void set_status(uint8_t value) {
    outb((uint16_t)(net.io_base + 18u), value);
}

static uint8_t read_status(void) {
    return inb((uint16_t)(net.io_base + 18u));
}

static void select_queue(uint16_t q) {
    outw((uint16_t)(net.io_base + 14u), q);
}

static void set_queue_size(uint16_t size) {
    outw((uint16_t)(net.io_base + 12u), size);
}

static void set_queue_pfn(uint32_t pfn) {
    outl((uint16_t)(net.io_base + 8u), pfn);
}

static void notify_queue(uint16_t q) {
    outw((uint16_t)(net.io_base + 16u), q);
}

static void read_mac(void) {
    /* Legacy virtio-net device config sits right after the common registers. */
    for (int i = 0; i < 6; ++i) {
        net.mac[i] = inb((uint16_t)(net.io_base + 20u + (uint16_t)i));
    }
}

static void memcpy8(uint8_t *dst, const uint8_t *src, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        dst[i] = src[i];
    }
}

static int setup_queue(uint16_t q, vring_desc_t **desc_out, vring_avail_t **avail_out,
                       vring_used_t **used_out) {
    vring_desc_t *desc = (vring_desc_t *)memory_dma_alloc(sizeof(vring_desc_t) * NET_RING_SIZE, PAGE_SIZE);
    vring_avail_t *avail = (vring_avail_t *)memory_dma_alloc(sizeof(vring_avail_t), PAGE_SIZE);
    vring_used_t *used = (vring_used_t *)memory_dma_alloc(sizeof(vring_used_t), PAGE_SIZE);

    if (!desc || !avail || !used) {
        return -1;
    }

    for (size_t i = 0; i < NET_RING_SIZE; ++i) {
        desc[i].addr = 0;
        desc[i].len = 0;
        desc[i].flags = 0;
        desc[i].next = 0;
    }
    avail->flags = 0;
    avail->idx = 0;
    used->flags = 0;
    used->idx = 0;

    select_queue(q);
    set_queue_size(NET_RING_SIZE);
    set_queue_pfn((uint32_t)((uint64_t)(uintptr_t)desc >> 12));

    *desc_out = desc;
    *avail_out = avail;
    *used_out = used;
    return 0;
}

static void refill_rx(void) {
    while ((uint16_t)(net.rx_avail_idx - net.rx_last_used) < NET_RING_SIZE) {
        uint16_t slot = (uint16_t)(net.rx_avail_idx % NET_RING_SIZE);
        uint8_t *buf = (uint8_t *)memory_dma_alloc(NET_BUF_SIZE, PAGE_SIZE);
        if (!buf) {
            return;
        }
        net.rx_buf[slot] = buf;
        net.rx_desc[slot].addr = (uint64_t)(uintptr_t)buf;
        net.rx_desc[slot].len = NET_BUF_SIZE;
        net.rx_desc[slot].flags = VRING_DESC_F_WRITE;
        net.rx_desc[slot].next = 0;
        net.rx_avail->ring[slot] = slot;
        net.rx_avail->idx = (uint16_t)(net.rx_avail->idx + 1);
        net.rx_avail_idx++;
    }
    notify_queue(NET_QUEUE_RX);
}

static uint16_t ip_checksum(const uint8_t *data, size_t len) {
    uint32_t sum = 0;
    for (size_t i = 0; i + 1 < len; i += 2) {
        sum += (uint32_t)((uint16_t)data[i] << 8) | data[i + 1];
    }
    if (len & 1) {
        sum += (uint32_t)((uint16_t)data[len - 1] << 8);
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFFu) + (sum >> 16);
    }
    return (uint16_t)(~sum & 0xFFFFu);
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

static uint16_t get_u16_be(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static uint32_t get_u32_be(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

static int send_frame(const uint8_t *dst_mac, uint16_t ethertype, const uint8_t *payload, size_t len) {
    if (!net.present || len + NET_HDR_SIZE + 14 > NET_BUF_SIZE) {
        return -1;
    }

    uint8_t *buf = net.tx_buf;
    for (size_t i = 0; i < NET_HDR_SIZE; ++i) {
        buf[i] = 0;
    }
    memcpy8(buf + NET_HDR_SIZE, dst_mac, 6);
    memcpy8(buf + NET_HDR_SIZE + 6, net.mac, 6);
    put_u16_be(buf + NET_HDR_SIZE + 12, ethertype);
    memcpy8(buf + NET_HDR_SIZE + 14, payload, len);

    size_t total = NET_HDR_SIZE + 14 + len;

    net.tx_desc[0].addr = (uint64_t)(uintptr_t)buf;
    net.tx_desc[0].len = (uint32_t)total;
    net.tx_desc[0].flags = 0;
    net.tx_desc[0].next = 0;

    uint16_t slot = (uint16_t)(net.tx_avail->idx % NET_RING_SIZE);
    net.tx_avail->ring[slot] = 0;
    net.tx_avail->idx = (uint16_t)(net.tx_avail->idx + 1);

    notify_queue(NET_QUEUE_TX);
    net.tx_pkts++;

    /* Wait briefly for the device to consume the TX descriptor. */
    for (int spin = 0; spin < 100000; ++spin) {
        if (net.tx_used->idx != net.tx_last_used) {
            net.tx_last_used = net.tx_used->idx;
            break;
        }
    }
    return 0;
}

static void send_arp_reply(const uint8_t *req) {
    /* req points at the ARP payload (starts after the 14-byte ethernet header). */
    uint8_t reply[28];
    put_u16_be(reply + 0, 1);      /* hardware type: ethernet */
    put_u16_be(reply + 2, 0x0800); /* protocol type: IPv4 */
    reply[4] = 6;                  /* hardware addr len */
    reply[5] = 4;                  /* protocol addr len */
    put_u16_be(reply + 6, 2);      /* opcode: reply */
    memcpy8(reply + 8, net.mac, 6);
    put_u32_be(reply + 14, NET_LOCAL_IP);
    memcpy8(reply + 18, req + 8, 6);   /* requester MAC */
    put_u32_be(reply + 24, get_u32_be(req + 14));
    send_frame(req + 8, ETH_TYPE_ARP, reply, sizeof(reply));
    net.arp_replies++;
}

static void send_icmp_echo_reply(const uint8_t *ip_hdr, const uint8_t *icmp, size_t icmp_len) {
    uint8_t frame[NET_BUF_SIZE];
    uint8_t *ip = frame;
    size_t ip_hdr_len = (size_t)(ip_hdr[0] & 0x0F) * 4;
    if (ip_hdr_len < 20 || ip_hdr_len > 60) {
        return;
    }

    memcpy8(ip, ip_hdr, ip_hdr_len);
    /* Swap src/dst */
    uint32_t src = get_u32_be(ip_hdr + 12);
    uint32_t dst = get_u32_be(ip_hdr + 16);
    put_u32_be(ip + 12, dst);
    put_u32_be(ip + 16, src);
    ip[9] = IP_PROTO_ICMP;
    /* total length unchanged */
    ip[10] = 0;
    ip[11] = 0;
    uint16_t csum = ip_checksum(ip, ip_hdr_len);
    ip[10] = (uint8_t)(csum >> 8);
    ip[11] = (uint8_t)(csum & 0xFF);

    uint8_t *out_icmp = ip + ip_hdr_len;
    memcpy8(out_icmp, icmp, icmp_len);
    out_icmp[0] = ICMP_ECHO_REPLY;
    out_icmp[2] = 0;
    out_icmp[3] = 0;
    uint16_t ic = ip_checksum(out_icmp, icmp_len);
    out_icmp[2] = (uint8_t)(ic >> 8);
    out_icmp[3] = (uint8_t)(ic & 0xFF);

    /* Use the source MAC from the request ethernet frame if known; broadcast fallback. */
    /* We approximate: reply to the requester's MAC. The caller passes the ethernet frame
       header just before ip_hdr, so walk back 14 bytes. */
    const uint8_t *eth = ip_hdr - 14;
    send_frame(eth + 6, ETH_TYPE_IPV4, ip, ip_hdr_len + icmp_len);
    net.icmp_replies++;
}

static void handle_packet(const uint8_t *buf, size_t len) {
    if (len < 14) {
        return;
    }
    uint16_t ethertype = get_u16_be(buf + 12);
    const uint8_t *payload = buf + 14;
    size_t payload_len = len - 14;

    if (ethertype == ETH_TYPE_ARP && payload_len >= 28) {
        uint16_t op = get_u16_be(payload + 6);
        uint32_t sender_ip = get_u32_be(payload + 14);
        if (op == 1 && get_u32_be(payload + 24) == NET_LOCAL_IP) {
            send_arp_reply(payload);
        }
        if ((op == 1 || op == 2) && sender_ip == NET_GATEWAY_IP && !net.gateway_resolved) {
            memcpy8(net.gateway_mac, payload + 8, 6);
            net.gateway_resolved = 1;
        }
        return;
    }

    if (ethertype == ETH_TYPE_IPV4 && payload_len >= 20) {
        size_t ip_hdr_len = (size_t)(payload[0] & 0x0F) * 4;
        if (ip_hdr_len < 20 || ip_hdr_len > payload_len) {
            return;
        }
        uint32_t dst_ip = get_u32_be(payload + 16);
        if (dst_ip != NET_LOCAL_IP) {
            return;
        }
        uint8_t proto = payload[9];
        const uint8_t *l4 = payload + ip_hdr_len;
        size_t l4_len = payload_len - ip_hdr_len;

        if (proto == IP_PROTO_ICMP) {
            if (l4_len < 8) {
                return;
            }
            if (l4[0] == ICMP_ECHO_REQUEST) {
                send_icmp_echo_reply(payload, l4, l4_len);
            }
            if (l4[0] == ICMP_ECHO_REPLY && get_u32_be(payload + 12) == net.ping_target_ip) {
                net.ping_got_reply = 1;
                net.ping_replies++;
            }
            return;
        }

        if (proto == IP_PROTO_TCP && net.tcp_input_fn) {
            uint32_t src_ip = get_u32_be(payload + 12);
            net.tcp_input_fn(src_ip, l4, l4_len);
            return;
        }

        if (proto == IP_PROTO_UDP) {
            uint32_t src_ip = get_u32_be(payload + 12);
            if (l4_len >= 8) {
                dns_input(src_ip, l4 + 8, l4_len - 8);
            }
            return;
        }
    }
}

void net_poll(void) {
    if (!net.present) {
        return;
    }
    while (net.rx_used->idx != net.rx_last_used) {
        uint16_t slot = (uint16_t)(net.rx_last_used % NET_RING_SIZE);
        vring_used_elem_t elem = net.rx_used->ring[slot];
        uint8_t *buf = net.rx_buf[slot];
        size_t len = (size_t)elem.len;
        if (buf && len > NET_HDR_SIZE) {
            handle_packet(buf + NET_HDR_SIZE, len - NET_HDR_SIZE);
            net.rx_pkts++;
        }
        net.rx_last_used++;
        /* Recycle the buffer into a fresh RX descriptor. */
        net.rx_desc[slot].addr = (uint64_t)(uintptr_t)buf;
        net.rx_desc[slot].len = NET_BUF_SIZE;
        net.rx_desc[slot].flags = VRING_DESC_F_WRITE;
        net.rx_desc[slot].next = 0;
        net.rx_avail->ring[(uint16_t)(net.rx_avail->idx % NET_RING_SIZE)] = slot;
        net.rx_avail->idx = (uint16_t)(net.rx_avail->idx + 1);
        net.rx_avail_idx++;
    }
    notify_queue(NET_QUEUE_RX);
}

int net_send_arp(uint32_t target_ip) {
    uint8_t arp[28];
    uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    put_u16_be(arp + 0, 1);
    put_u16_be(arp + 2, 0x0800);
    arp[4] = 6;
    arp[5] = 4;
    put_u16_be(arp + 6, 1);
    memcpy8(arp + 8, net.mac, 6);
    put_u32_be(arp + 14, NET_LOCAL_IP);
    for (int i = 18; i < 24; ++i) {
        arp[i] = 0;
    }
    put_u32_be(arp + 24, target_ip);
    return send_frame(broadcast, ETH_TYPE_ARP, arp, sizeof(arp));
}

int net_ping(uint32_t ip) {
    if (!net.present) {
        return -1;
    }

    net.ping_target_ip = ip;
    net.ping_got_reply = 0;
    net.ping_sent++;

    uint8_t icmp[8];
    icmp[0] = ICMP_ECHO_REQUEST;
    icmp[1] = 0;
    icmp[2] = 0;
    icmp[3] = 0;
    put_u16_be(icmp + 4, 0x0001);  /* identifier */
    put_u16_be(icmp + 6, 0x0001);  /* sequence */
    uint16_t ic = ip_checksum(icmp, 8);
    icmp[2] = (uint8_t)(ic >> 8);
    icmp[3] = (uint8_t)(ic & 0xFF);

    uint8_t ip_hdr[20];
    ip_hdr[0] = 0x45;
    ip_hdr[1] = 0;
    put_u16_be(ip_hdr + 2, (uint16_t)(20 + 8));
    put_u16_be(ip_hdr + 4, 0);
    put_u16_be(ip_hdr + 6, 0);
    ip_hdr[8] = 64;
    ip_hdr[9] = IP_PROTO_ICMP;
    ip_hdr[10] = 0;
    ip_hdr[11] = 0;
    put_u32_be(ip_hdr + 12, NET_LOCAL_IP);
    put_u32_be(ip_hdr + 16, ip);
    uint16_t csum = ip_checksum(ip_hdr, 20);
    ip_hdr[10] = (uint8_t)(csum >> 8);
    ip_hdr[11] = (uint8_t)(csum & 0xFF);

    uint8_t frame[28];
    memcpy8(frame, ip_hdr, 20);
    memcpy8(frame + 20, icmp, 8);

    /* Resolve gateway MAC when pinging the QEMU user-net gateway. */
    net_send_arp(ip);
    if (ip == NET_GATEWAY_IP) {
        for (int i = 0; i < 50; ++i) {
            net_poll();
            if (net.gateway_resolved) {
                break;
            }
            for (volatile int s = 0; s < 10000; ++s) { }
        }
    }

    /* We don't keep an ARP cache MAC per host yet, so broadcast the ICMP.
       Real hosts on the QEMU user-net still deliver it to the target. */
    uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    send_frame(broadcast, ETH_TYPE_IPV4, frame, 28);

    for (int i = 0; i < 100; ++i) {
        net_poll();
        if (net.ping_got_reply) {
            break;
        }
        for (volatile int s = 0; s < 20000; ++s) { }
    }

    return net.ping_got_reply ? 0 : -1;
}

void net_set_tcp_input(void (*fn)(uint32_t src_ip, const uint8_t *pkt, size_t len)) {
    net.tcp_input_fn = fn;
}

const uint8_t *net_resolve_gateway(void) {
    if (!net.present) {
        return 0;
    }
    if (net.gateway_resolved) {
        return net.gateway_mac;
    }
    net_send_arp(NET_GATEWAY_IP);
    for (int i = 0; i < 200; ++i) {
        net_poll();
        if (net.gateway_resolved) {
            return net.gateway_mac;
        }
        for (volatile int s = 0; s < 20000; ++s) { }
    }
    return 0;
}

int net_send_ip(uint32_t dst_ip, uint8_t proto, const uint8_t *payload, size_t len) {
    if (!net.present) {
        return -1;
    }
    if (len + 20u > NET_BUF_SIZE - NET_HDR_SIZE - 14u) {
        return -1;
    }
    const uint8_t *gw = net_resolve_gateway();
    if (!gw) {
        return -1;
    }

    uint8_t *buf = net.tx_buf;
    for (size_t i = 0; i < NET_HDR_SIZE; ++i) {
        buf[i] = 0;
    }
    memcpy8(buf + NET_HDR_SIZE, gw, 6);
    memcpy8(buf + NET_HDR_SIZE + 6, net.mac, 6);
    put_u16_be(buf + NET_HDR_SIZE + 12, ETH_TYPE_IPV4);

    uint8_t *ip = buf + NET_HDR_SIZE + 14;
    size_t total_ip = 20 + len;
    ip[0] = 0x45;
    ip[1] = 0;
    put_u16_be(ip + 2, (uint16_t)total_ip);
    put_u16_be(ip + 4, 0);
    put_u16_be(ip + 6, 0x4000);  /* don't fragment */
    ip[8] = 64;
    ip[9] = proto;
    ip[10] = 0;
    ip[11] = 0;
    put_u32_be(ip + 12, NET_LOCAL_IP);
    put_u32_be(ip + 16, dst_ip);
    uint16_t csum = ip_checksum(ip, 20);
    ip[10] = (uint8_t)(csum >> 8);
    ip[11] = (uint8_t)(csum & 0xFF);
    memcpy8(ip + 20, payload, len);

    size_t frame_len = NET_HDR_SIZE + 14 + total_ip;
    net.tx_desc[0].addr = (uint64_t)(uintptr_t)buf;
    net.tx_desc[0].len = (uint32_t)frame_len;
    net.tx_desc[0].flags = 0;
    net.tx_desc[0].next = 0;
    uint16_t slot = (uint16_t)(net.tx_avail->idx % NET_RING_SIZE);
    net.tx_avail->ring[slot] = 0;
    net.tx_avail->idx = (uint16_t)(net.tx_avail->idx + 1);
    notify_queue(NET_QUEUE_TX);
    net.tx_pkts++;
    for (int spin = 0; spin < 100000; ++spin) {
        if (net.tx_used->idx != net.tx_last_used) {
            net.tx_last_used = net.tx_used->idx;
            break;
        }
    }
    return 0;
}

int net_send_udp(uint32_t dst_ip, uint16_t dst_port, uint16_t src_port,
                 const uint8_t *payload, size_t len) {
    uint8_t udp[512];
    size_t total = 8 + len;

    if (!net.present || total > sizeof(udp)) {
        return -1;
    }
    put_u16_be(udp + 0, src_port);
    put_u16_be(udp + 2, dst_port);
    put_u16_be(udp + 4, (uint16_t)total);
    put_u16_be(udp + 6, 0);
    for (size_t i = 0; i < len; ++i) {
        udp[8 + i] = payload[i];
    }
    return net_send_ip(dst_ip, IP_PROTO_UDP, udp, total);
}

int net_present(void) {
    return net.present;
}

uint32_t net_local_ip(void) {
    return NET_LOCAL_IP;
}

const uint8_t *net_mac(void) {
    return net.mac;
}

int net_parse_ip(const char *text, uint32_t *out) {
    uint32_t parts[4];
    int idx = 0;
    uint32_t cur = 0;
    int got = 0;

    for (size_t i = 0; text[i] && idx < 4; ++i) {
        char c = text[i];
        if (c >= '0' && c <= '9') {
            cur = cur * 10u + (uint32_t)(c - '0');
            got = 1;
            if (cur > 255) {
                return -1;
            }
        } else if (c == '.') {
            if (!got) {
                return -1;
            }
            parts[idx++] = cur;
            cur = 0;
            got = 0;
        } else {
            return -1;
        }
    }
    if (!got || idx != 3) {
        return -1;
    }
    parts[idx] = cur;

    *out = (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8) | parts[3];
    return 0;
}

static int name_equal_ci(const char *a, const char *b) {
    while (*a && *b) {
        char ca = *a;
        char cb = *b;
        if (ca >= 'A' && ca <= 'Z') {
            ca = (char)(ca + 32);
        }
        if (cb >= 'A' && cb <= 'Z') {
            cb = (char)(cb + 32);
        }
        if (ca != cb) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

int net_resolve_host(const char *host, uint32_t *out) {
    static const struct {
        const char *name;
        uint32_t ip;
    } hosts[] = {
        {"localhost", 0x7F000001u},
        {"gateway", 0x0A000202u},
        {"gw", 0x0A000202u},
        {"grid", 0x0A00020Fu},
        {"host", 0x0A00020Fu},
        {"ai", 0x0A000202u},
        {"btc", 0x0A000202u},
        {0, 0},
    };

    if (!host || !out) {
        return -1;
    }
    if (net_parse_ip(host, out) == 0) {
        return 0;
    }
    for (int i = 0; hosts[i].name; ++i) {
        if (name_equal_ci(host, hosts[i].name)) {
            *out = hosts[i].ip;
            return 0;
        }
    }
    if (hosts_lookup(host, out) == 0) {
        return 0;
    }
    if (dns_resolve(host, out) == 0) {
        return 0;
    }
    return -1;
}

void net_print_status(void) {
    console_write("Network:   ");
    if (!net.present) {
        console_set_color(GRID_COL_DIM);
        console_write_line("no virtio-net device");
        console_set_color(GRID_COL_DEFAULT);
        return;
    }
    console_set_color(GRID_COL_OK);
    console_write_line("virtio-net online");
    console_set_color(GRID_COL_DEFAULT);
    console_write("  MAC:        ");
    for (int i = 0; i < 6; ++i) {
        static const char hex[] = "0123456789ABCDEF";
        console_write_char(hex[(net.mac[i] >> 4) & 0xF]);
        console_write_char(hex[net.mac[i] & 0xF]);
        if (i < 5) {
            console_write_char(':');
        }
    }
    console_write_char('\n');
    console_write("  IP:         10.0.2.15\n");
    console_write("  Gateway:    10.0.2.2 (also: gateway, gw)\n");
    console_write("  DNS names:  grid, host, localhost, ai, btc, /etc/hosts, UDP\n");
    console_write("  RX packets: ");
    {
        char buf[16];
        size_t pos = 0;
        uint32_t v = net.rx_pkts;
        if (v == 0) { buf[pos++] = '0'; }
        while (v > 0) { buf[pos++] = (char)('0' + (v % 10)); v /= 10; }
        for (size_t i = 0; i < pos / 2; ++i) { char t = buf[i]; buf[i] = buf[pos - 1 - i]; buf[pos - 1 - i] = t; }
        buf[pos] = '\0';
        console_write_line(buf);
    }
    console_write("  TX packets: ");
    {
        char buf[16];
        size_t pos = 0;
        uint32_t v = net.tx_pkts;
        if (v == 0) { buf[pos++] = '0'; }
        while (v > 0) { buf[pos++] = (char)('0' + (v % 10)); v /= 10; }
        for (size_t i = 0; i < pos / 2; ++i) { char t = buf[i]; buf[i] = buf[pos - 1 - i]; buf[pos - 1 - i] = t; }
        buf[pos] = '\0';
        console_write_line(buf);
    }
    console_write_line("  Use: net ping <ip>  (e.g. 10.0.2.2)");
}

static void append_str(char *out, size_t cap, size_t *pos, const char *s) {
    if (!out || !pos || !s) {
        return;
    }
    while (*s && *pos + 1 < cap) {
        out[(*pos)++] = *s++;
    }
    if (*pos < cap) {
        out[*pos] = '\0';
    }
}

int net_format_ip(uint32_t ip, char *out, size_t out_len) {
    if (!out || out_len < 8) {
        return 0;
    }
    size_t pos = 0;
    for (int b = 3; b >= 0; --b) {
        uint8_t byte = (uint8_t)((ip >> (b * 8)) & 0xFFu);
        if (b < 3 && pos + 1 < out_len) {
            out[pos++] = '.';
        }
        if (pos + 2 < out_len) {
            out[pos++] = (char)('0' + (byte / 100u) % 10u);
            out[pos++] = (char)('0' + (byte / 10u) % 10u);
            out[pos++] = (char)('0' + byte % 10u);
        }
    }
    out[pos] = '\0';
    return (int)pos;
}

int net_format_status(char *out, size_t out_len) {
    if (!out || out_len == 0) {
        return 0;
    }
    size_t pos = 0;
    out[0] = '\0';
    if (!net.present) {
        append_str(out, out_len, &pos, "offline");
        return (int)pos;
    }
    append_str(out, out_len, &pos, "online ip=10.0.2.15 gw=10.0.2.2");
    return (int)pos;
}

void net_init(void) {
    pci_device_t dev;
    net.present = 0;

    if (pci_find_device(VIRTIO_VENDOR_ID, VIRTIO_DEV_NET_LEGACY, &dev) != 0) {
        return;
    }

    int is_io = 0;
    uint64_t bar = pci_get_bar(&dev, 0, &is_io);
    if (!is_io || bar == 0) {
        return;
    }

    net.pci = dev;
    net.io_base = (uint16_t)bar;

    set_status(0);
    set_status(VIRTIO_STATUS_ACK);
    set_status((uint8_t)(VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER));
    /* Negotiate minimal features: MAC and status. No GSO/CSUM/MRG. */
    uint32_t feats = host_features();
    (void)feats;
    guest_features(0);
    set_status((uint8_t)(VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK));
    if ((read_status() & VIRTIO_STATUS_FEATURES_OK) == 0) {
        /* Some QEMU builds are lenient about FEATURES_OK on legacy; continue anyway. */
    }

    read_mac();

    if (setup_queue(NET_QUEUE_RX, &net.rx_desc, &net.rx_avail, &net.rx_used) != 0) {
        return;
    }
    if (setup_queue(NET_QUEUE_TX, &net.tx_desc, &net.tx_avail, &net.tx_used) != 0) {
        return;
    }

    net.tx_buf = (uint8_t *)memory_dma_alloc(NET_BUF_SIZE, PAGE_SIZE);
    if (!net.tx_buf) {
        return;
    }

    net.rx_last_used = 0;
    net.tx_last_used = 0;
    net.rx_avail_idx = 0;
    net.rx_pkts = 0;
    net.tx_pkts = 0;
    net.arp_replies = 0;
    net.icmp_replies = 0;
    net.ping_sent = 0;
    net.ping_replies = 0;
    net.ping_target_ip = 0;
    net.ping_got_reply = 0;
    net.gateway_resolved = 0;
    net.tcp_input_fn = 0;

    refill_rx();

    set_status((uint8_t)(VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_DRIVER_OK));
    net.present = 1;
}
