#ifndef TCP_H
#define TCP_H

#include <stddef.h>
#include <stdint.h>

#define TCP_FIN 0x01u
#define TCP_SYN 0x02u
#define TCP_RST 0x04u
#define TCP_PSH 0x08u
#define TCP_ACK 0x10u

/* Global TCP pool — shared by GRID.SERVER, IRC server/client, HTTP, AI/BTC
 * bridges. No per-subsystem quota; each service competes for slots. */
#define TCP_MAX_CONNECTIONS 12
#define TCP_MAX_LISTENERS   4
#define TCP_MAX_PENDING     4

typedef struct {
    uint32_t remote_ip;
    uint16_t remote_port;
    uint16_t local_port;
    uint32_t tx_seq;
    uint32_t rx_seq;
    int established;
    int closed;
    int error;
    /* inbound delivery buffer */
    uint8_t rx_buf[4096];
    size_t rx_len;
} tcp_conn_t;

void tcp_init(void);
int tcp_listen(uint16_t port);
void tcp_unlisten(uint16_t port);
int tcp_listen_active(uint16_t port);
int tcp_connect(tcp_conn_t *c, uint32_t ip, uint16_t port);
int tcp_accept(tcp_conn_t **out);
int tcp_accept_port(tcp_conn_t **out, uint16_t local_port);
int tcp_send(tcp_conn_t *c, const void *data, size_t len);
size_t tcp_peek(const tcp_conn_t *c, void *out, size_t cap);
size_t tcp_consume(tcp_conn_t *c, size_t n);
int tcp_recv(tcp_conn_t *c, uint32_t timeout_loops);
void tcp_close(tcp_conn_t *c);
void tcp_input(uint32_t src_ip, const uint8_t *pkt, size_t len);

#endif
