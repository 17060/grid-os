#include "btc.h"

#include "net.h"
#include "tcp.h"

#include <stddef.h>
#include <stdint.h>

#define BTC_HOST_IP  0x0A000202u  /* 10.0.2.2 */
#define BTC_TCP_PORT 8767u
#define BTC_HDR      "GRIDBTC/1.0/"
#define BTC_OK       "GRIDBTC/1.0/OK"
#define BTC_ERR      "GRIDBTC/1.0/ERR"
#define BTC_END      "#GRIDBTC/END"

static size_t btc_strlen(const char *s) {
    size_t n = 0;
    if (!s) {
        return 0;
    }
    while (s[n]) {
        n++;
    }
    return n;
}

static void btc_strcpy(char *d, size_t cap, const char *s) {
    size_t n = 0;
    if (!d || cap == 0) {
        return;
    }
    if (!s) {
        d[0] = '\0';
        return;
    }
    while (s[n] && n + 1 < cap) {
        d[n] = s[n];
        n++;
    }
    d[n] = '\0';
}

static int btc_streq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

static int btc_contains(const char *hay, const char *needle) {
    size_t nlen = btc_strlen(needle);
    if (nlen == 0) {
        return 0;
    }
    for (size_t i = 0; hay[i]; ++i) {
        size_t j = 0;
        while (j < nlen && hay[i + j] == needle[j]) {
            j++;
        }
        if (j == nlen) {
            return 1;
        }
    }
    return 0;
}

static size_t build_request(const char *method, const char *params, char *buf, size_t cap) {
    size_t p = 0;
    const char *hdr = BTC_HDR;
    while (*hdr && p + 1 < cap) {
        buf[p++] = *hdr++;
    }
    if (method) {
        for (size_t i = 0; method[i] && p + 1 < cap; ++i) {
            buf[p++] = method[i];
        }
    }
    if (p + 1 < cap) {
        buf[p++] = '\n';
    }
    if (params) {
        for (size_t i = 0; params[i] && p + 1 < cap; ++i) {
            char c = params[i];
            if (c == '\r') {
                continue;
            }
            buf[p++] = c;
        }
    }
    if (p + 1 < cap) {
        buf[p++] = '\n';
    }
    const char *end = BTC_END;
    while (*end && p + 1 < cap) {
        buf[p++] = *end++;
    }
    if (p + 1 < cap) {
        buf[p++] = '\n';
    }
    buf[p] = '\0';
    return p;
}

static int parse_response(const char *raw, char *out, size_t cap) {
    const char *p = raw;
    int in_body = 0;

    out[0] = '\0';
    while (*p) {
        const char *line_start = p;
        while (*p && *p != '\n') {
            p++;
        }
        char line[180];
        size_t len = (size_t)(p - line_start);
        if (len >= sizeof(line)) {
            len = sizeof(line) - 1;
        }
        for (size_t i = 0; i < len; ++i) {
            line[i] = line_start[i];
        }
        line[len] = '\0';
        if (*p == '\n') {
            p++;
        }

        if (btc_streq(line, BTC_END)) {
            return in_body == 1 ? 0 : -1;
        }
        if (btc_streq(line, BTC_OK)) {
            in_body = 1;
            continue;
        }
        if (btc_streq(line, BTC_ERR)) {
            in_body = 2;
            continue;
        }
        if (in_body == 1) {
            if (out[0]) {
                size_t ol = btc_strlen(out);
                if (ol + 2 < cap) {
                    out[ol] = '\n';
                    out[ol + 1] = '\0';
                }
            }
            size_t ol = btc_strlen(out);
            for (size_t i = 0; line[i] && ol + 1 < cap; ++i) {
                out[ol++] = line[i];
            }
            out[ol] = '\0';
        } else if (in_body == 2) {
            btc_strcpy(out, cap, line);
            return -1;
        }
    }
    return in_body == 1 ? 0 : -1;
}

static void offline_error(char *out, size_t cap) {
    btc_strcpy(out, cap,
               "Grid BTC offline: bridge unreachable (host: make btc-bridge, TCP :8767). "
               "Set BITCOIN_RPC_* and run Bitcoin Core on the host.");
}

static int try_tcp_bridge(const char *method, const char *params, char *out, size_t cap) {
    char req[768];
    size_t req_len;
    tcp_conn_t conn;
    char accum[BTC_RESP_MAX];
    size_t acc = 0;
    int got_end = 0;

    if (!net_present()) {
        return -1;
    }

    req_len = build_request(method, params, req, sizeof(req));
    if (tcp_connect(&conn, BTC_HOST_IP, (uint16_t)BTC_TCP_PORT) != 0) {
        return -1;
    }

    if (tcp_send(&conn, req, req_len) != 0) {
        tcp_close(&conn);
        return -1;
    }

    accum[0] = '\0';
    for (int round = 0; round < 400 && !got_end; ++round) {
        int n = tcp_recv(&conn, 512);
        if (n > 0 && conn.rx_len > 0) {
            for (size_t i = 0; i < conn.rx_len && acc + 1 < sizeof(accum); ++i) {
                accum[acc++] = (char)conn.rx_buf[i];
            }
            accum[acc] = '\0';
            conn.rx_len = 0;
            if (btc_contains(accum, BTC_END)) {
                got_end = 1;
            }
        }
        if (conn.closed || conn.error) {
            break;
        }
    }
    tcp_close(&conn);

    if (!got_end) {
        return -1;
    }
    return parse_response(accum, out, cap);
}

static int btc_dispatch(const char *method, const char *params, char *out, size_t cap) {
    if (try_tcp_bridge(method, params, out, cap) == 0) {
        return 0;
    }
    offline_error(out, cap);
    return -1;
}

int btc_call(const char *method, const char *params, char *out, size_t cap) {
    return btc_dispatch(method ? method : "", params ? params : "", out, cap);
}

void btc_status(char *out, size_t cap) {
    if (try_tcp_bridge("STATUS", "", out, cap) == 0) {
        return;
    }
    offline_error(out, cap);
}

int btc_blockchain(char *out, size_t cap) {
    return btc_call("getblockchaininfo", "", out, cap);
}

int btc_network(char *out, size_t cap) {
    return btc_call("getnetworkinfo", "", out, cap);
}

int btc_wallet(char *out, size_t cap) {
    return btc_call("getwalletinfo", "", out, cap);
}

int btc_balance(char *out, size_t cap) {
    return btc_call("getbalance", "", out, cap);
}

int btc_address(const char *label, char *out, size_t cap) {
    if (label && label[0]) {
        char params[128];
        size_t p = 0;
        params[p++] = '[';
        params[p++] = '"';
        for (size_t i = 0; label[i] && p + 4 < sizeof(params); ++i) {
            if (label[i] == '"' || label[i] == '\\') {
                if (p + 2 >= sizeof(params)) {
                    break;
                }
                params[p++] = '\\';
            }
            params[p++] = label[i];
        }
        if (p + 2 < sizeof(params)) {
            params[p++] = '"';
            params[p++] = ']';
            params[p] = '\0';
            return btc_call("getnewaddress", params, out, cap);
        }
    }
    return btc_call("getnewaddress", "", out, cap);
}

int btc_send(const char *addr, const char *amount, char *out, size_t cap) {
    char params[256];
    size_t p = 0;
    if (!addr || !amount) {
        btc_strcpy(out, cap, "error: need address and amount");
        return -1;
    }
    params[p++] = '[';
    params[p++] = '"';
    for (size_t i = 0; addr[i] && p + 6 < sizeof(params); ++i) {
        if (addr[i] == '"' || addr[i] == '\\') {
            params[p++] = '\\';
        }
        params[p++] = addr[i];
    }
    params[p++] = '"';
    params[p++] = ',';
    for (size_t i = 0; amount[i] && p + 2 < sizeof(params); ++i) {
        char c = amount[i];
        if ((c < '0' || c > '9') && c != '.') {
            btc_strcpy(out, cap, "error: amount must be numeric");
            return -1;
        }
        params[p++] = c;
    }
    params[p++] = ']';
    params[p] = '\0';
    return btc_call("sendtoaddress", params, out, cap);
}

int btc_tx(const char *txid, char *out, size_t cap) {
    char params[128];
    size_t p = 0;
    if (!txid || !txid[0]) {
        btc_strcpy(out, cap, "error: need txid");
        return -1;
    }
    params[p++] = '[';
    params[p++] = '"';
    for (size_t i = 0; txid[i] && p + 4 < sizeof(params); ++i) {
        params[p++] = txid[i];
    }
    params[p++] = '"';
    params[p++] = ']';
    params[p] = '\0';
    return btc_call("getrawtransaction", params, out, cap);
}

int btc_block(const char *hash_or_height, char *out, size_t cap) {
    char params[128];
    char blockhash[128];
    size_t p = 0;
    int all_digits = 1;

    if (!hash_or_height || !hash_or_height[0]) {
        btc_strcpy(out, cap, "error: need block hash or height");
        return -1;
    }
    for (size_t i = 0; hash_or_height[i]; ++i) {
        if (hash_or_height[i] < '0' || hash_or_height[i] > '9') {
            all_digits = 0;
            break;
        }
    }
    if (all_digits) {
        params[p++] = '[';
        for (size_t i = 0; hash_or_height[i] && p + 2 < sizeof(params); ++i) {
            params[p++] = hash_or_height[i];
        }
        params[p++] = ']';
        params[p] = '\0';
        if (btc_call("getblockhash", params, blockhash, sizeof(blockhash)) != 0) {
            btc_strcpy(out, cap, blockhash);
            return -1;
        }
        hash_or_height = blockhash;
    }
    p = 0;
    params[p++] = '[';
    params[p++] = '"';
    /* Reserve 5 bytes for the 4-char suffix ("," 1 ]) plus the NUL below;
     * a 4-byte reservation let params[p]='\0' write one past params[128]. */
    for (size_t i = 0; hash_or_height[i] && p + 5 < sizeof(params); ++i) {
        params[p++] = hash_or_height[i];
    }
    params[p++] = '"';
    params[p++] = ',';
    params[p++] = '1';
    params[p++] = ']';
    params[p] = '\0';
    return btc_call("getblock", params, out, cap);
}

int btc_help(char *out, size_t cap) {
    return btc_call("HELP", "", out, cap);
}

int btc_stop(char *out, size_t cap) {
    return btc_call("stop", "", out, cap);
}
