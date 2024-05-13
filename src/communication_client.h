#ifndef SRC_COMMUNICATION_CLIENT_H_
#define SRC_COMMUNICATION_CLIENT_H_

#include <arpa/inet.h>
#include <netinet/in.h>

#include "./messages.h"
#include "./model.h"

int send_initial_connexion_information(int sock, GAME_MODE mode);
int send_ready_connexion_information(int sock, GAME_MODE mode, int id, int eq);

connection_information *recv_connexion_information(int sock);

/* TODO: think about this */
typedef struct udp_information {
    int sock;
    struct sockaddr_in6 addr;
} udp_information;

#endif // SRC_COMMUNICATION_CLIENT_H_
