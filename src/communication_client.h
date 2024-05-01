#ifndef SRC_COMMUNICATION_CLIENT_H_
#define SRC_COMMUNICATION_CLIENT_H_

#include <arpa/inet.h>

#include "./messages.h"
#include "./model.h"

int send_initial_connexion_information(int sock, GAME_MODE mode);
int send_ready_connexion_information(int sock, GAME_MODE mode, int id, int eq);

connection_information *recv_connexion_information(int sock);

/* TODO: think about this */
typedef struct upd_information {
    int sock;
    struct sockaddr_in *addr;
    socklen_t *addr_len;
} udp_information;

typedef enum game_message_type {
    GAME_BOARD_INFORMATION,
    GAME_BOARD_UPDATE,
} game_message_type;

typedef struct received_game_message {
    char *message;
    game_message_type type;
} received_game_message;

received_game_message *recv_game_message(udp_information *info);

#endif // SRC_COMMUNICATION_CLIENT_H_
