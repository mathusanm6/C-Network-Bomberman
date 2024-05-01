#ifndef SRC_COMMUNICATION_CLIENT_H
#define SRC_COMMUNICATION_CLIENT_H

#include <arpa/inet.h>

#include "./messages.h"
#include "./model.h"

int send_initial_connexion_information(int sock, GAME_MODE mode);
int send_ready_connexion_information(int sock, GAME_MODE mode, int id, int eq);

connection_information *recv_connexion_information(int sock);

typedef enum game_message_type {
    GAME_BOARD_INFORMATION,
    GAME_BOARD_UPDATE,
} game_message_type;

typedef struct recieved_game_message {
    char *message;
    game_message_type type;
} recieved_game_message;

/* TODO: think about this */
typedef struct upd_information {
    int sock;
    struct sockaddr_in *addr;
    socklen_t *addr_len;
} udp_information;

recieved_game_message *recv_game_message(udp_information *);

#endif // SRC_COMMUNICATION_CLIENT_H
