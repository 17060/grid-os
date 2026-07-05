#ifndef NET_H
#define NET_H

#include <stddef.h>
#include <stdint.h>

void net_init(void);
int net_present(void);
void net_poll(void);
int net_ping(uint32_t ip);
int net_send_arp(uint32_t target_ip);
void net_print_status(void);
int net_parse_ip(const char *text, uint32_t *out);
uint32_t net_local_ip(void);
const uint8_t *net_mac(void);

/* Raw IP plumbing for the TCP layer. */
int net_send_ip(uint32_t dst_ip, uint8_t proto, const uint8_t *payload, size_t len);
const uint8_t *net_resolve_gateway(void);
void net_set_tcp_input(void (*fn)(uint32_t src_ip, const uint8_t *pkt, size_t len));

#endif
