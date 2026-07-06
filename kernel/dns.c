#include "dns.h"
#include "gfs.h"
#include "net.h"

#include <stddef.h>
#include <stdint.h>

#define HOSTS_MAX 24
#define HOSTS_NAME_MAX 32
#define DNS_GATEWAY 0x0A000202u
#define DNS_PORT 53u

typedef struct {
    int used;
    char name[HOSTS_NAME_MAX];
    uint32_t ip;
} hosts_entry_t;

static hosts_entry_t g_hosts[HOSTS_MAX];
static int g_hosts_loaded;
static uint16_t g_dns_txid = 0x4742u;
static volatile int g_dns_done;
static volatile uint32_t g_dns_answer;
static volatile uint16_t g_dns_match_id;

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

static void skip_ws(const char **p) {
    while (**p == ' ' || **p == '\t') {
        (*p)++;
    }
}

static void hosts_clear(void) {
    for (int i = 0; i < HOSTS_MAX; ++i) {
        g_hosts[i].used = 0;
    }
}

void hosts_reload(void) {
    g_hosts_loaded = 0;
}

static void hosts_load_once(void) {
    char buf[512];
    size_t len = 0;

    if (g_hosts_loaded) {
        return;
    }
    g_hosts_loaded = 1;
    hosts_clear();

    if (!gfs_present()) {
        return;
    }
    if (gfs_read_file("/etc/hosts", buf, sizeof(buf) - 1, &len) != 0 || len == 0) {
        return;
    }
    buf[len] = '\0';

    const char *line = buf;
    while (*line) {
        const char *end = line;
        while (*end && *end != '\n') {
            end++;
        }
        char scratch[128];
        size_t n = (size_t)(end - line);
        if (n >= sizeof(scratch)) {
            n = sizeof(scratch) - 1;
        }
        for (size_t i = 0; i < n; ++i) {
            scratch[i] = line[i];
        }
        scratch[n] = '\0';

        const char *p = scratch;
        skip_ws(&p);
        if (*p == '#' || *p == '\0') {
            line = (*end == '\n') ? end + 1 : end;
            continue;
        }

        char ipbuf[20];
        size_t ipi = 0;
        while (*p && *p != ' ' && *p != '\t' && ipi + 1 < sizeof(ipbuf)) {
            ipbuf[ipi++] = *p++;
        }
        ipbuf[ipi] = '\0';
        uint32_t ip = 0;
        if (net_parse_ip(ipbuf, &ip) != 0) {
            line = (*end == '\n') ? end + 1 : end;
            continue;
        }

        while (*p) {
            skip_ws(&p);
            if (*p == '#' || *p == '\0') {
                break;
            }
            char name[HOSTS_NAME_MAX];
            size_t ni = 0;
            while (*p && *p != ' ' && *p != '\t' && *p != '#' && ni + 1 < sizeof(name)) {
                name[ni++] = *p++;
            }
            name[ni] = '\0';
            if (ni > 0) {
                for (int i = 0; i < HOSTS_MAX; ++i) {
                    if (!g_hosts[i].used) {
                        g_hosts[i].used = 1;
                        for (size_t k = 0; k < ni; ++k) {
                            g_hosts[i].name[k] = name[k];
                        }
                        g_hosts[i].name[ni] = '\0';
                        g_hosts[i].ip = ip;
                        break;
                    }
                }
            }
        }
        line = (*end == '\n') ? end + 1 : end;
    }
}

int hosts_lookup(const char *name, uint32_t *out_ip) {
    if (!name || !out_ip) {
        return -1;
    }
    hosts_load_once();
    for (int i = 0; i < HOSTS_MAX; ++i) {
        if (g_hosts[i].used && name_equal_ci(name, g_hosts[i].name)) {
            *out_ip = g_hosts[i].ip;
            return 0;
        }
    }
    return -1;
}

static size_t encode_dns_name(const char *host, uint8_t *out, size_t cap) {
    size_t pos = 0;
    const char *p = host;

    while (*p) {
        const char *label = p;
        while (*p && *p != '.') {
            p++;
        }
        size_t len = (size_t)(p - label);
        if (len == 0 || len > 63 || pos + 1 + len >= cap) {
            return 0;
        }
        out[pos++] = (uint8_t)len;
        for (size_t i = 0; i < len; ++i) {
            out[pos++] = (uint8_t)label[i];
        }
        if (*p == '.') {
            p++;
        }
    }
    if (pos + 1 >= cap) {
        return 0;
    }
    out[pos++] = 0;
    return pos;
}

static void put_u16_be(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)(v & 0xFF);
}

static uint16_t get_u16_be(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static uint32_t get_u32_be(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

void dns_init(void) {
    g_dns_done = 0;
    g_dns_answer = 0;
    g_hosts_loaded = 0;
}

void dns_input(uint32_t src_ip, const uint8_t *udp_payload, size_t len) {
    if (src_ip != DNS_GATEWAY || len < 12) {
        return;
    }
    uint16_t id = get_u16_be(udp_payload + 0);
    if (id != g_dns_match_id) {
        return;
    }
    uint16_t flags = get_u16_be(udp_payload + 2);
    if ((flags & 0x8000u) == 0) {
        return;
    }
    uint16_t ancount = get_u16_be(udp_payload + 6);
    if (ancount == 0) {
        return;
    }

    size_t i = 12;
    while (i < len && udp_payload[i] != 0) {
        if ((udp_payload[i] & 0xC0u) == 0xC0u) {
            i += 2;
            break;
        }
        i += 1 + udp_payload[i];
    }
    if (i < len && udp_payload[i] == 0) {
        i++;
    }
    i += 4;

    for (uint16_t a = 0; a < ancount && i + 10 < len; ++a) {
        if ((udp_payload[i] & 0xC0u) == 0xC0u) {
            i += 2;
        } else {
            while (i < len && udp_payload[i] != 0) {
                i += 1 + udp_payload[i];
            }
            if (i < len) {
                i++;
            }
        }
        if (i + 10 > len) {
            break;
        }
        uint16_t rtype = get_u16_be(udp_payload + i);
        uint16_t rdlen = get_u16_be(udp_payload + i + 8);
        i += 10;
        if (rtype == 1 && rdlen == 4 && i + 4 <= len) {
            g_dns_answer = get_u32_be(udp_payload + i);
            g_dns_done = 1;
            return;
        }
        i += rdlen;
    }
}

int dns_resolve(const char *hostname, uint32_t *out_ip) {
    uint8_t packet[256];
    size_t pos = 0;

    if (!hostname || !out_ip || !net_present()) {
        return -1;
    }
    if (hostname[0] == '\0') {
        return -1;
    }

    g_dns_txid++;
    g_dns_match_id = g_dns_txid;
    g_dns_done = 0;
    g_dns_answer = 0;

    put_u16_be(packet + 0, g_dns_txid);
    put_u16_be(packet + 2, 0x0100u);
    put_u16_be(packet + 4, 1);
    put_u16_be(packet + 6, 0);
    put_u16_be(packet + 8, 0);
    put_u16_be(packet + 10, 0);
    pos = 12;

    size_t nlen = encode_dns_name(hostname, packet + pos, sizeof(packet) - pos - 4);
    if (nlen == 0) {
        return -1;
    }
    pos += nlen;
    put_u16_be(packet + pos, 1);
    put_u16_be(packet + pos + 2, 1);
    pos += 4;

    if (net_send_udp(DNS_GATEWAY, DNS_PORT, 0xDE00u, packet, pos) != 0) {
        return -1;
    }

    for (int tries = 0; tries < 400; ++tries) {
        net_poll();
        if (g_dns_done) {
            *out_ip = g_dns_answer;
            return 0;
        }
        for (volatile int s = 0; s < 10000; ++s) { }
    }
    return -1;
}
