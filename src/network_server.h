#ifndef SRC_NETWORK_SERVER_H_
#define SRC_NETWORK_SERVER_H_

#include "communication_server.h"

#define MIN_PORT 1024
#define MAX_PORT 49151

/** Parameter for a server managing a single game */
typedef struct server_information {
    int sock_udp;
    int sock_mult;

    int sock_clients[PLAYER_NUM]; // socket TCP to send game informations

    uint16_t port_udp;
    uint16_t port_mult;

    uint16_t adrmdiff[8]; // Multicast address
    struct sockaddr_in6 *addr_mult;
} server_information;

int init_socket_tcp();
server_information *init_server_network(uint16_t connexion_port);
int game_loop_server(server_information *server);

#endif // SRC_NETWORK_SERVER_H__H_
