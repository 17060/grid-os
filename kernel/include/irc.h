#ifndef IRC_H
#define IRC_H

#include <stddef.h>
#include <stdint.h>

#define IRC_LINE_MAX 512

int irc_connect(const char *host_ip, uint16_t port, const char *nick);
void irc_disconnect(void);
int irc_join(const char *channel);
int irc_part(const char *channel);
int irc_say(const char *target, const char *msg);
int irc_nick(const char *nick);
int irc_quit(const char *reason);
void irc_poll(void);
size_t irc_read(char *buf, size_t cap);
void irc_status(char *buf, size_t cap);
int irc_is_connected(void);

/* Legacy one-shot listen session (connect + join + poll loop). */
int irc_session(const char *host_ip, uint16_t port, const char *nick, const char *channel,
                uint32_t duration_loops);

#endif
