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

#include "model.h"

#define MAX_PORT_TRY 250
#define MIN_PORT 1024
#define MAX_PORT 49151

typedef struct tcp_thread_data {
    pthread_t thread_id;
    int client_sock_tcp;
    bool finished_flag;
} tcp_thread_data;

static int sock_tcp = 0;
static uint16_t port = 0;

static tcp_thread_data *tcp_threads_data[PLAYER_NUM];

static unsigned connected_player_number = 0;

pthread_mutex_t lock_waiting_all_players_join;
pthread_mutex_t lock_all_players_ready;

pthread_cond_t cond_lock_waiting_all_payers_join;
pthread_cond_t cond_lock_all_players_ready;

void *serve_client(void *);

void close_tcp_socket() {
    if (sock_tcp != 0) {
        close(sock_tcp);
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

int init_tcp_socket() {
    sock_tcp = socket(PF_INET6, SOCK_STREAM, 0);
    if (sock_tcp < 0) {
        perror("tcp socket creation");
        sock_tcp = 0;
        return EXIT_FAILURE;
    }
    int no = 0;
    if (setsockopt(sock_tcp, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no)) < 0) {
        perror("setsockopt polymorphism");
        close_tcp_socket();
        sock_tcp = 0;
        return EXIT_FAILURE;
    }
    int ok = 1;
    if (setsockopt(sock_tcp, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(ok)) < 0) {
        perror("setsockopt reuseaddr");
        close_tcp_socket();
        sock_tcp = 0;
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

uint16_t get_random_port() {
    return htons(MIN_PORT + (random() % (MAX_PORT - MIN_PORT)));
}

int try_to_bind_port_on_tcp_socket() {
    struct sockaddr_in6 adrsock;
    memset(&adrsock, 0, sizeof(adrsock));
    adrsock.sin6_family = AF_INET6;
    adrsock.sin6_addr = in6addr_any;

    for (unsigned i = 0; i < MAX_PORT_TRY; i++) {
        uint16_t random_port = get_random_port();
        adrsock.sin6_port = random_port;

        if (bind(sock_tcp, (struct sockaddr *)&adrsock, sizeof(adrsock)) < 0) {
            if (errno != EADDRINUSE) {
                break;
            }

        } else {
            port = random_port;
            return EXIT_SUCCESS;
        }
    }
    return EXIT_FAILURE;
}

int connect_player_to_game() {
    while (connected_player_number < PLAYER_NUM) {

        tcp_threads_data[connected_player_number]->client_sock_tcp = accept(sock_tcp, NULL, NULL);
        if (tcp_threads_data[connected_player_number]->client_sock_tcp < 0) {
            perror("client acceptation");
            return EXIT_FAILURE;
        }
        if (pthread_create(&tcp_threads_data[connected_player_number]->thread_id, NULL, serve_client,
                           tcp_threads_data[connected_player_number]) < 0) {
            perror("thread creation");
            return EXIT_FAILURE;
        }
        connected_player_number++;
    }
    sleep(1);
    // TODO Change place of this
    pthread_mutex_lock(&lock_waiting_all_players_join);
    pthread_cond_broadcast(&cond_lock_waiting_all_payers_join);
    pthread_mutex_unlock(&lock_waiting_all_players_join);
    return EXIT_SUCCESS;
}

void *serve_client(void *arg_tcp_thread_data) {
    tcp_thread_data *tcp_data = (tcp_thread_data *)arg_tcp_thread_data;

    // TODO read client connection
    pthread_mutex_lock(&lock_waiting_all_players_join);
    pthread_cond_wait(&cond_lock_waiting_all_payers_join, &lock_waiting_all_players_join);
    pthread_mutex_unlock(&lock_waiting_all_players_join);

    // TODO send information of connection

    // TODO read client is ready
    pthread_mutex_lock(&lock_all_players_ready);
    pthread_cond_wait(&cond_lock_all_players_ready, &lock_all_players_ready);
    pthread_mutex_unlock(&lock_all_players_ready);

    return NULL;
}

int main() {
    srandom(time(NULL));
    if (init_tcp_socket() == EXIT_FAILURE || try_to_bind_port_on_tcp_socket() == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }
    printf("PORT TCP : %d\n", ntohs(port));
    close_tcp_socket();
    return EXIT_SUCCESS;
}
