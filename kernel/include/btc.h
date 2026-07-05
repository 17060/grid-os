#ifndef BTC_H
#define BTC_H

#include <stddef.h>

#define BTC_RESP_MAX 4096u

/* Host bridge (TCP 10.0.2.2:8767) with offline error fallback. */
int btc_call(const char *method, const char *params, char *out, size_t cap);
void btc_status(char *out, size_t cap);
int btc_blockchain(char *out, size_t cap);
int btc_network(char *out, size_t cap);
int btc_wallet(char *out, size_t cap);
int btc_balance(char *out, size_t cap);
int btc_address(const char *label, char *out, size_t cap);
int btc_send(const char *addr, const char *amount, char *out, size_t cap);
int btc_tx(const char *txid, char *out, size_t cap);
int btc_block(const char *hash_or_height, char *out, size_t cap);
int btc_help(char *out, size_t cap);
int btc_stop(char *out, size_t cap);

#endif
