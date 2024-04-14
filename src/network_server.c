#include "network_server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_PORT_TRY 250
#define START_ADRMDIFF 0xff12

/** Parameter for a server managing a single game, will change in the future to manage multiple game
 */
static int sock_tcp = -1;
static int sock_udp = -1;
static int sock_mult = -1;
static int sock_clients[PLAYER_NUM]; // socket TCP to send game informations

static uint16_t port_tcp = 0;
static uint16_t port_udp = 0;
static uint16_t port_mult = 0;

static uint16_t adrmdiff[8]; // Multicast address

void close_socket(int sock) {
    if (sock != -1) {
        close(sock);
    }
}

void close_socket_tcp() {
    return close_socket(sock_tcp);
}

void close_socket_udp() {
    return close_socket(sock_udp);
}

void close_socket_mult() {
    return close_socket(sock_mult);
}

void close_socket_client(int id) {
    if (id >= 0 && id < PLAYER_NUM) {
        return close_socket(sock_clients[id]);
    }
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
        close(*sock);
        *sock = -1;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int init_socket_tcp() {
    return init_socket(&sock_tcp, true);
}

int init_socket_udp() {
    return init_socket(&sock_udp, false);
}

int init_socket_mult() {
    return init_socket(&sock_mult, false);
}

void print_ip_of_client(struct sockaddr_in6 client_addr) {
    char client_ip[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &(client_addr.sin6_addr), client_ip, INET6_ADDRSTRLEN);
    printf("%s\n", client_ip);
}

int try_to_init_socket_of_client(int id) {
    if (id < 0 || id >= PLAYER_NUM) {
        return EXIT_FAILURE;
    }
    struct sockaddr_in6 client_addr;
    socklen_t client_addr_len;
    int res = accept(sock_tcp, (struct sockaddr *)&client_addr, &client_addr_len);
    if (res < 0) {
        perror("client acceptance");
        return EXIT_FAILURE;
    }
    sock_clients[id] = res;

    printf("Player %d is connected with the IP : ", id + 1);
    print_ip_of_client(client_addr);
    return EXIT_SUCCESS;
}

uint16_t get_random_port() {
    return htons(MIN_PORT + (random() % (MAX_PORT - MIN_PORT)));
}

uint16_t get_port_tcp() {
    return ntohs(port_tcp);
}

int try_to_bind_random_port_on_socket(int sock) {
    struct sockaddr_in6 adrsock;
    memset(&adrsock, 0, sizeof(adrsock));
    adrsock.sin6_family = AF_INET6;
    adrsock.sin6_addr = in6addr_any;

    for (unsigned i = 0; i < MAX_PORT_TRY; i++) {
        uint16_t random_port = get_random_port();
        adrsock.sin6_port = random_port;

        if (bind(sock, (struct sockaddr *)&adrsock, sizeof(adrsock)) < 0) {
            if (errno != EADDRINUSE) {
                break;
            }
        } else {
            return random_port;
        }
    }
    return EXIT_FAILURE;
}

int try_to_bind_random_port_on_socket_tcp() {
    int res = try_to_bind_random_port_on_socket(sock_tcp);
    if (res == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }
    port_tcp = res;
    return EXIT_SUCCESS;
}

int try_to_bind_random_port_on_socket_udp() {
    int res = try_to_bind_random_port_on_socket(sock_udp);
    if (res == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }
    port_udp = res;
    return EXIT_SUCCESS;
}

int init_random_port_on_socket_mult() {
    port_mult = get_random_port();
    return EXIT_SUCCESS;
}

int init_random_adrmdiff() {
    adrmdiff[0] = START_ADRMDIFF;

    unsigned size_2_bytes = 65536;
    for (unsigned i = 1; i < 8; i++) {
        adrmdiff[i] = random() % size_2_bytes;
    }

    return EXIT_SUCCESS;
}

int listen_players() {
    if (listen(sock_tcp, 0) < 0) {
        perror("listen sock_tcp");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

initial_connection_header *recv_initial_connection_header_of_client(int id) {
    return recv_initial_connection_header(sock_clients[id]);
}

ready_connection_header *recv_ready_connexion_header_of_client(int id) {
    return recv_ready_connexion_header(sock_clients[id]);
}

int send_connexion_information_of_client(int id, int eq) {
    return send_connexion_information(sock_clients[id], get_game_mode(), id, eq, port_udp, port_mult, adrmdiff);
}
