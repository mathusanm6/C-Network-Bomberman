#ifndef SRC_COMMUNICATION_CLIENT_H_
#define SRC_COMMUNICATION_CLIENT_H_

#include <arpa/inet.h>
#include <netinet/in.h>

#include "./messages.h"
#include "./model.h"

int send_initial_connexion_information(int sock, GAME_MODE mode);
int send_ready_connexion_information(int sock, GAME_MODE mode, int id, int eq);
int send_chat_message(int sock, chat_message_type type, int id, int eq, uint8_t message_length, char *message);

connection_information *recv_connexion_information(int sock);

/* TODO: think about this */
typedef struct udp_information {
    int sock;
    struct sockaddr_in6 addr;
} udp_information;

chat_message *recv_chat_message(int sock);
u_int16_t recv_header(int sock);

#endif // SRC_COMMUNICATION_CLIENT_H_
