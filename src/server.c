#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "communication.h"
#include "messages.h"
#include "model.h"

#define MAX_PORT_TRY 250
#define MIN_PORT 1024
#define MAX_PORT 49151

typedef struct tcp_thread_data {
    unsigned id;
    pthread_t thread_id;
    int client_sock_tcp;
    bool finished_flag;
} tcp_thread_data;

static int sock_tcp = -1;
static int sock_udp = -1;
static int sock_mult = -1;

static uint16_t port_tcp = 0;
static uint16_t port_udp = 0;
static uint16_t port_mult = 0;

static uint16_t adrmdiff[8];

static tcp_thread_data *tcp_threads_data[PLAYER_NUM];

static unsigned connected_player_number = 0;
static unsigned ready_player_number = 0;

pthread_mutex_t lock_waiting_all_players_join;
pthread_mutex_t lock_all_players_ready;

pthread_cond_t cond_lock_waiting_all_payers_join;
pthread_cond_t cond_lock_all_players_ready;

void *serve_client(void *);

void close_socket(int sock) {
    if (sock != -1) {
        close(sock);
    }
}

void free_tcp_threads_data() {
    for (unsigned i = 0; i < connected_player_number; i++) {
        if (tcp_threads_data[i] != NULL) {
            free(tcp_threads_data[i]);
            tcp_threads_data[i] = NULL;
        }
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
    int no = 0;
    if (setsockopt(*sock, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no)) < 0) {
        perror("setsockopt polymorphism");
        close_socket(*sock);
        *sock = -1;
        return EXIT_FAILURE;
    }
    int ok = 1;
    if (setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(ok)) < 0) {
        perror("setsockopt reuseaddr");
        close(*sock);
        *sock = -1;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int init_tcp_threads_data() {
    for (unsigned i = 0; i < PLAYER_NUM; i++) {
        tcp_threads_data[i] = malloc(sizeof(tcp_thread_data));

        if (tcp_threads_data[i] == NULL) {
            perror("malloc tcp_thread_data");
            for (unsigned j = 0; j < i; j++) {
                free(tcp_threads_data[j]);
            }
            return EXIT_FAILURE;
        }
        memset(&tcp_threads_data[i], 0, sizeof(tcp_thread_data));
    }
    return EXIT_SUCCESS;
}

int init_adrmdiff(uint16_t gradr[8]) {
    if (gradr == NULL) {
        return EXIT_FAILURE;
    }
    gradr[0] = 0xff12;

    unsigned size_2_byte = 65536;
    for (unsigned i = 1; i < 8; i++) {
        gradr[i] = random() % size_2_byte;
    }

    return EXIT_SUCCESS;
}

uint16_t get_random_port() {
    return htons(MIN_PORT + (random() % (MAX_PORT - MIN_PORT)));
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
    return 1;
}


void *serve_client(void *arg_tcp_thread_data) {
    tcp_thread_data *tcp_data = (tcp_thread_data *)arg_tcp_thread_data;
    
    // TODO separate solo and eq client
    recv_initial_connection_header(tcp_data->client_sock_tcp);
    pthread_mutex_lock(&lock_waiting_all_players_join);
    pthread_cond_wait(&cond_lock_waiting_all_payers_join, &lock_waiting_all_players_join);
    pthread_mutex_unlock(&lock_waiting_all_players_join);

    send_connexion_information(tcp_data->client_sock_tcp, get_game_mode(), tcp_data->id, tcp_data->id, port_udp, port_mult, adrmdiff);

    // TODO verify ready_informations
    ready_connection_header *ready_informations = recv_ready_connexion_header(tcp_data->client_sock_tcp);
    printf("Id : %d, eq : %d\n", ready_informations->id, ready_informations->eq);
    pthread_mutex_lock(&lock_all_players_ready);
    ready_player_number++;
    if (ready_player_number < PLAYER_NUM) {
        pthread_cond_wait(&cond_lock_all_players_ready, &lock_all_players_ready);
    } else {
        pthread_cond_broadcast(&cond_lock_all_players_ready);
    }
    pthread_mutex_unlock(&lock_all_players_ready);

    printf("Player %id leave\n", ready_informations->id);
    close_socket(tcp_data->client_sock_tcp);
    free(tcp_data);
    // TO CONTINUE
    return NULL;
}

int connect_player_to_game() {
    if (listen(sock_tcp, 0) < 0) {
        perror("listen sock_tcp");
        return EXIT_FAILURE;
    }
    printf("Waiting players on %u port.\n", ntohs(port_tcp));
    while (connected_player_number < PLAYER_NUM) {
        tcp_threads_data[connected_player_number] = malloc(sizeof(tcp_threads_data));
        tcp_threads_data[connected_player_number]->id = connected_player_number;

        int res = accept(sock_tcp, NULL, NULL);
        if (res < 0) {
            perror("client acceptation");
            return EXIT_FAILURE;
        }

        tcp_threads_data[connected_player_number]->client_sock_tcp = res;
        if (pthread_create(&tcp_threads_data[connected_player_number]->thread_id, NULL, serve_client,
                           tcp_threads_data[connected_player_number]) < 0) {
            perror("thread creation");
            return EXIT_FAILURE;
        }
        printf("Player %d is connected\n", connected_player_number + 1);
        connected_player_number++;
    }
    printf("All players are connected.\n");
    return EXIT_SUCCESS;
}

void unlock_all_players_join() {
    pthread_mutex_lock(&lock_waiting_all_players_join);
    pthread_cond_broadcast(&cond_lock_waiting_all_payers_join);
    pthread_mutex_unlock(&lock_waiting_all_players_join);
}

void join_tcp_threads() {
    for (unsigned i = 0; i < PLAYER_NUM; i++) {
        pthread_join(tcp_threads_data[i]->thread_id, NULL);
    }
}


int main() {
    srandom(time(NULL));
    if (init_socket(&sock_tcp, true) == EXIT_FAILURE || init_socket(&sock_udp, false) ||
        init_socket(&sock_mult, false)) {
        return EXIT_FAILURE;
    }
    port_tcp = try_to_bind_random_port_on_socket(sock_tcp);
    if (port_tcp == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }
    port_udp = try_to_bind_random_port_on_socket(sock_udp);
    if (port_udp == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }
    port_mult = get_random_port();
    if (init_adrmdiff(adrmdiff) == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }
    if (connect_player_to_game() == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }
    sleep(1);
    unlock_all_players_join();
    join_tcp_threads();

    close_socket(sock_tcp);
    close_socket(sock_udp);
    close_socket(sock_mult);
    return EXIT_SUCCESS;
}
