/* Host-side DNS resolver tests (mirrors kernel net_resolve_host). */

#include <stdio.h>
#include <stdint.h>

static int net_parse_ip(const char *text, uint32_t *out) {
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

static int net_resolve_host(const char *host, uint32_t *out) {
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
    return -1;
}

int main(void) {
    uint32_t ip;

    if (net_resolve_host("10.0.2.2", &ip) != 0 || ip != 0x0A000202u) {
        fprintf(stderr, "literal IP resolve failed\n");
        return 1;
    }
    if (net_resolve_host("gateway", &ip) != 0 || ip != 0x0A000202u) {
        fprintf(stderr, "gateway resolve failed\n");
        return 1;
    }
    if (net_resolve_host("GRID", &ip) != 0 || ip != 0x0A00020Fu) {
        fprintf(stderr, "grid case-insensitive resolve failed\n");
        return 1;
    }
    if (net_resolve_host("unknown.example", &ip) == 0) {
        fprintf(stderr, "unknown host should fail\n");
        return 1;
    }
    printf("net host tests OK\n");
    return 0;
}
