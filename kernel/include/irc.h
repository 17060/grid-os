#ifndef IRC_H
#define IRC_H

#include <stdint.h>

int irc_session(const char *host_ip, uint16_t port, const char *nick, const char *channel,
                uint32_t duration_loops);

#endif
