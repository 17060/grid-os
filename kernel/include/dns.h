#ifndef DNS_H
#define DNS_H

#include <stddef.h>
#include <stdint.h>

void dns_init(void);
void dns_input(uint32_t src_ip, const uint8_t *udp_payload, size_t len);
int dns_resolve(const char *hostname, uint32_t *out_ip);
int hosts_lookup(const char *name, uint32_t *out_ip);
void hosts_reload(void);

#endif
