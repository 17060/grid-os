#ifndef IRC_SERVER_H
#define IRC_SERVER_H

#include <stddef.h>
#include <stdint.h>

#define GRID_IRC_SERVER_SLOTS      8
#define GRID_IRC_SERVER_LISTEN_MAX 4
#define GRID_IRC_SERVER_EVENTS     32
#define GRID_IRC_SERVER_LINE_MAX   512
#define GRID_IRC_SERVER_NICK_MAX   32
#define GRID_IRC_SERVER_CHAN_MAX   64

void grid_irc_server_init(void);
int grid_irc_server_listen(uint16_t port);
int grid_irc_server_unlisten(uint16_t port);
void grid_irc_server_stop_all(void);
void grid_irc_server_poll(void);
int grid_irc_server_listening(uint16_t port);

/* Event queue: PRIVMSG|slot|nick|target|text or JOIN|slot|nick|chan */
int grid_irc_server_event(char *out, size_t cap);
const char *grid_irc_server_event_kind(void);
int grid_irc_server_event_slot(void);
void grid_irc_server_event_nick(char *out, size_t cap);
void grid_irc_server_event_target(char *out, size_t cap);
void grid_irc_server_event_text(char *out, size_t cap);

int grid_irc_server_say(int slot, const char *target, const char *text);
int grid_irc_server_notice(int slot, const char *target, const char *text);
int grid_irc_server_bot_say(const char *target, const char *text);
int grid_irc_server_bot_notice(const char *target, const char *text);
void grid_irc_server_format_status(char *out, size_t cap);
void grid_irc_server_client_nick(int slot, char *out, size_t cap);

#endif
