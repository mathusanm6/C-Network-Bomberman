#ifndef SRC_NETWORK_SERVER_H_
#define SRC_NETWORK_SERVER_H_

#include "communication_server.h"

#define MIN_PORT 1024
#define MAX_PORT 49151

int init_server_network();
int game_loop_server();

#endif // SRC_NETWORK_SERVER_H__H_
