#include "network_client.h"
#include "communication_client.h"
#include "messages.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *IP_SERVER = "::1";

static int sock_tcp = -1;

static uint16_t port_tcp = 0;
static uint16_t port_udp = 0;
static uint16_t port_mult = 0;

static uint16_t adrmdiff[8];

static int id;
static int eq;

void close_socket(int sock) {
    if (sock != 0) {
        close(sock);
    }
}

void close_socket_tcp() {
    close(sock_tcp);
}

int init_socket(int *sock, bool is_tcp) {
    if (is_tcp) {
        *sock = socket(PF_INET6, SOCK_STREAM, 0);
    } else {
        *sock = socket(PF_INET6, SOCK_DGRAM, 0);
    }
    if (*sock < 0) {
        perror("socket creation");
        *sock = -1;
        return EXIT_FAILURE;
    }
    int option = 0; // Option value for setsockopt
    if (setsockopt(*sock, IPPROTO_IPV6, IPV6_V6ONLY, &option, sizeof(option)) < 0) {
        perror("setsockopt polymorphism");
        close_socket(*sock);
        *sock = -1;
        return EXIT_FAILURE;
    }
    option = 1;
    if (setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
        perror("setsockopt reuseaddr");
        close_socket(*sock);
        *sock = -1;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int init_tcp_socket() {
    return init_socket(&sock_tcp, true);
}

void set_tcp_port(uint16_t port) {
    port_tcp = htons(port);
}

struct sockaddr_in6 *prepare_address(int port) {
    struct sockaddr_in6 *addrsock = malloc(sizeof(struct sockaddr_in6));
    if (addrsock == NULL) {
        perror("malloc addrsock");
        return NULL;
    }
    memset(addrsock, 0, sizeof(struct sockaddr_in6));
    addrsock->sin6_family = AF_INET6;
    addrsock->sin6_port = port;
    inet_pton(AF_INET6, IP_SERVER, &addrsock->sin6_addr);
    return addrsock;
}

int try_to_connect_tcp() {
    struct sockaddr_in6 *addrsock = prepare_address(port_tcp);
    return connect(sock_tcp, (struct sockaddr *)addrsock, sizeof(struct sockaddr_in6));
}

void set_server_informations(connection_information *head) {
    if (head == NULL) {
        return;
    }
    port_udp = head->portudp;
    port_mult = head->portmdiff;
    id = head->id;
    eq = head->eq;
    for (unsigned i = 0; i < 8; i++) {
        adrmdiff[i] = head->adrmdiff[i];
    }
}

int start_initialisation_game(GAME_MODE mode) {
    if (send_initial_connexion_information(sock_tcp, mode) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    printf("You have to wait for other players.\n");
    connection_information *head = recv_connexion_information(sock_tcp);
    if (head == NULL) {
        return EXIT_FAILURE;
    }
    set_server_informations(head);
    free(head);
    printf("The server is ready.\n");
    return EXIT_SUCCESS;
}

int send_ready_to_play(GAME_MODE mode) {
    if (send_ready_connexion_information(sock_tcp, mode, id, eq) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
