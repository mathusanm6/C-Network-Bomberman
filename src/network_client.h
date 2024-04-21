#ifndef SRC_NETWORK_CLIENT_H_
#define SRC_NETWORK_CLIENT_H_

#include "model.h"
#include <stdint.h>

#define MIN_PORT 1024
#define MAX_PORT 49151

int init_tcp_socket();
void close_socket_tcp();
void set_tcp_port(uint16_t port);
int try_to_connect_tcp();
int start_initialisation_game(GAME_MODE mode);
int send_ready_to_play(GAME_MODE mode);

#endif // SRC_NETWORK_CLIENT_H__H_
