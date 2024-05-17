#ifndef SRC_NETWORK_CLIENT_H_
#define SRC_NETWORK_CLIENT_H_

#include "messages.h"
#include "model.h"
#include <stdint.h>

#define MIN_PORT 1024
#define MAX_PORT 49151

int init_tcp_socket();
void close_socket_tcp();
void set_tcp_port(uint16_t port);
int try_to_connect_tcp();
connection_information *start_initialisation_game(GAME_MODE mode);
int send_ready_to_play(GAME_MODE mode);

typedef enum game_message_type {
    GAME_BOARD_INFORMATION,
    GAME_BOARD_UPDATE,
} game_message_type;

typedef struct received_game_message {
    char *message;
    game_message_type type;
} received_game_message;

received_game_message *recv_game_message();
int send_game_action(game_action *action);

#endif // SRC_NETWORK_CLIENT_H__H_
