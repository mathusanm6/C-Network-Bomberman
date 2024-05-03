#ifndef SRC_COMMUNICATION_CLIENT_H_
#define SRC_COMMUNICATION_CLIENT_H_

#include <arpa/inet.h>

#include "./messages.h"
#include "./model.h"

int send_initial_connexion_information(int sock, GAME_MODE mode);
int send_ready_connexion_information(int sock, GAME_MODE mode, int id, int eq);

connection_information *recv_connexion_information(int sock);

/* TODO: think about this */
typedef struct udp_information {
    int sock;
    struct sockaddr *addr;
    socklen_t *addr_len;
} udp_information;

message_header *recv_header_multidiff(const udp_information *info);

void recvfrom_full(const udp_information *info, char *buffer, int size);

#endif // SRC_COMMUNICATION_CLIENT_H_
