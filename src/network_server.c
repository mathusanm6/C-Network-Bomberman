#include "network_server.h"
#include "messages.h"
#include "model.h"
#include "utils.h"

#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_PORT_TRY 250
#define START_ADRMDIFF 0xff12

#define ERROR_ADDRINUSE 2

#define LIMIT_LAST_NUM_MESSAGE_MULT ((1 << 15) - 1)   // 2^16
#define LIMIT_LAST_NUM_MESSAGE_CLIENT ((1 << 12) - 1) // 2^13

#define FREQ 50000 // 100 00 us = 10 ms
#define INITIAL_GAME_ACTIONS_SIZE 4
#define INITIAL_POLL_FD_SIZE 9

typedef struct tcp_thread_data {
    unsigned id;
    int game_id;
    int eq;
    unsigned *ready_player_number;
    unsigned *connected_players;

    int last_num_message;

    bool *finished_flag;
    unsigned *nb_players_left;

    pthread_mutex_t *lock_waiting_all_players_join;
    pthread_mutex_t *lock_all_players_ready;
    pthread_mutex_t *lock_waiting_the_game_finish;
    pthread_mutex_t *lock_all_tcp_threads_closed;
    pthread_mutex_t *lock_finished_flag;
    pthread_mutex_t *lock_nb_players_left;

    pthread_cond_t *cond_lock_waiting_all_players_join;
    pthread_cond_t *cond_lock_all_players_ready;
    pthread_cond_t *cond_lock_waiting_the_game_finish;
    pthread_cond_t *cond_lock_all_tcp_threads_closed;

    server_information *server;
} tcp_thread_data;

typedef struct udp_thread_data {
    unsigned game_id;
    game_action **game_actions;
    unsigned nb_game_actions;
    unsigned size_game_actions;

    bool *finished_flag;
    unsigned *nb_stopped_udp_threads;

    pthread_mutex_t lock_game_actions;
    pthread_mutex_t lock_send_udp;
    pthread_mutex_t *lock_finished_flag;
    pthread_mutex_t *lock_all_tcp_threads_closed;
    pthread_mutex_t *lock_nb_stopped_udp_threads;
    pthread_mutex_t *lock_all_udp_threads_closed;
    pthread_cond_t *cond_lock_all_tcp_threads_closed;
    pthread_cond_t *cond_lock_all_udp_threads_closed;

    server_information *server;
} udp_thread_data;

static int sock_tcp = -1;
static uint16_t port_tcp = -1;

static pthread_t game_threads[3];

static server_information *solo_waiting_server;
static int connected_solo_players = 0;
static tcp_thread_data *solo_tcp_threads_data_players[PLAYER_NUM];

static server_information *team_waiting_server;
static int connected_team_players = 0;
static tcp_thread_data *team_tcp_threads_data_players[PLAYER_NUM];

static int connection_port;

static pthread_mutex_t *lock_game_model;

void init_state(uint16_t connection_port_) {
    solo_waiting_server = NULL;
    team_waiting_server = NULL;

    connected_solo_players = 0;
    connected_team_players = 0;

    connection_port = connection_port_;
}

server_information *create_server_information() {
    server_information *server = malloc(sizeof(server_information));
    if (server == NULL) {
        perror("malloc server_information");
        return NULL;
    }

    server->sock_udp = -1;
    server->sock_mult = -1;
    for (int i = 0; i < PLAYER_NUM; i++) {
        server->sock_clients[i] = -1;
    }

    server->port_udp = 0;
    server->port_mult = 0;

    server->addr_mult = NULL;

    return server;
}

void close_socket(int sock) {
    if (sock != -1) {
        close(sock);
    }
}

void close_socket_tcp() {
    close_socket(sock_tcp);
}

void close_socket_udp(server_information *server) {
    shutdown(server->sock_udp, SHUT_RD);
    close_socket(server->sock_udp);
}

void close_socket_mult(server_information *server) {
    shutdown(server->sock_mult, SHUT_RD);
    close_socket(server->sock_mult);
}

void close_socket_client(server_information *server, int id) {
    if (id >= 0 && id < PLAYER_NUM) {
        if (server->sock_clients[id] != -1) {
            close(server->sock_clients[id]);
            server->sock_clients[id] = -1; // Mark socket as closed
        }
    }
}

int init_socket(int *sock, bool is_tcp) {
    printf("Creating socket\n");
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

    printf("Socket created\n");
    printf("Socket : %d\n", *sock);
    return EXIT_SUCCESS;
}

int try_to_bind_port_on_socket(int sock, struct sockaddr_in6 adrsock, uint16_t port) {
    adrsock.sin6_port = port;

    if (bind(sock, (struct sockaddr *)&adrsock, sizeof(adrsock)) < 0) {
        if (errno != EADDRINUSE) {
            return EXIT_FAILURE;
        }
        return ERROR_ADDRINUSE;
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

        int r = try_to_bind_port_on_socket(sock, adrsock, random_port);
        if (r == ERROR_ADDRINUSE) {
            continue;
        }
        if (r == EXIT_SUCCESS) {
            return random_port;
        }
        break;
    }
    return EXIT_FAILURE;
}

int try_to_bind_random_port_on_socket_tcp() {
    int res = try_to_bind_random_port_on_socket(sock_tcp);
    RETURN_FAILURE_IF_ERROR(res);
    port_tcp = res;
    return EXIT_SUCCESS;
}

int init_socket_tcp() {
    RETURN_FAILURE_IF_ERROR(init_socket(&sock_tcp, true));

    if (try_to_bind_random_port_on_socket_tcp() != EXIT_SUCCESS) {
        close_socket_tcp();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int init_socket_udp(server_information *server) {
    return init_socket(&server->sock_udp, false);
}

int init_socket_mult(server_information *server) {
    return init_socket(&server->sock_mult, false);
}

void print_ip_of_client(struct sockaddr_in6 client_addr) {
    char client_ip[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &(client_addr.sin6_addr), client_ip, INET6_ADDRSTRLEN);
    printf("%s\n", client_ip);
}

uint16_t get_port_tcp() {
    return ntohs(port_tcp);
}

int try_to_bind_port_on_socket_tcp(uint16_t connexion_port) {
    struct sockaddr_in6 adrsock;
    memset(&adrsock, 0, sizeof(adrsock));
    adrsock.sin6_family = AF_INET6;
    adrsock.sin6_addr = in6addr_any;
    uint16_t network_port = htons(connexion_port);

    if (try_to_bind_port_on_socket(sock_tcp, adrsock, network_port) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    port_tcp = network_port;
    return EXIT_SUCCESS;
}

int try_to_bind_random_port_on_socket_udp(server_information *server) {
    int res = try_to_bind_random_port_on_socket(server->sock_udp);
    RETURN_FAILURE_IF_ERROR(res);
    server->port_udp = res;
    return EXIT_SUCCESS;
}

int init_random_port_on_socket_mult(server_information *server) {
    server->port_mult = get_random_port();
    return EXIT_SUCCESS;
}

int init_random_adrmdiff(server_information *server) {
    server->adrmdiff[0] = START_ADRMDIFF;

    unsigned size_2_bytes = 65536;
    for (unsigned i = 1; i < 8; i++) {
        server->adrmdiff[i] = random() % size_2_bytes;
    }

    return EXIT_SUCCESS;
}

void free_addr_mult(server_information *server) {
    if (server->addr_mult != NULL) {
        free(server->addr_mult);
        server->addr_mult = NULL;
    }
}

int init_addr_mult(server_information *server) {
    server->addr_mult = malloc(sizeof(struct sockaddr_in6));
    RETURN_FAILURE_IF_NULL_PERROR(server->addr_mult, "malloc addr_mult");

    memset(server->addr_mult, 0, sizeof(struct sockaddr_in6));
    server->addr_mult->sin6_family = AF_INET6;
    server->addr_mult->sin6_port = server->port_mult;

    char *addr_string = convert_adrmdif_into_string(server->adrmdiff);
    int res = inet_pton(AF_INET6, addr_string, &server->addr_mult->sin6_addr);
    free(addr_string);

    if (res < 0) {
        free_addr_mult(server);
        perror("inet_pton addr_mult");
        return EXIT_FAILURE;
    }

    int ifindex = if_nametoindex("eth0");
    if (ifindex < 0) {
        free_addr_mult(server);
        perror("if_nametoindex eth0");
        return EXIT_FAILURE;
    }

    server->addr_mult->sin6_scope_id = ifindex;
    return EXIT_SUCCESS;
}

server_information *init_server_network(uint16_t connexion_port) {
    server_information *server = create_server_information();
    RETURN_NULL_IF_NULL(server);
    RETURN_NULL_IF_ERROR(init_socket_udp(server));
    RETURN_NULL_IF_ERROR(init_socket_mult(server));

    if (connexion_port >= MIN_PORT && connexion_port <= MAX_PORT) {
        if (try_to_bind_port_on_socket_tcp(connexion_port) != EXIT_SUCCESS) {
            fprintf(stderr, "The connexion port not works, try another one.\n");
            goto exit_closing_sockets;
        }
        // TODO: Move this elsewhere
    }

    if (try_to_bind_random_port_on_socket_udp(server) != EXIT_SUCCESS) {
        goto exit_closing_sockets;
    }
    if (init_random_port_on_socket_mult(server) != EXIT_SUCCESS) {
        goto exit_closing_sockets;
    }
    if (init_random_adrmdiff(server) == EXIT_FAILURE) {
        goto exit_closing_sockets;
    }
    if (init_addr_mult(server) == EXIT_FAILURE) {
        goto exit_closing_sockets;
    }
    return server;

exit_closing_sockets:
    close_socket_tcp();
    close_socket_udp(server);
    close_socket_mult(server);
    free(server);
    return NULL;
}

int listen_players() {
    if (listen(sock_tcp, 0) < 0) {
        perror("listen sock_tcp");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void free_tcp_threads_data(tcp_thread_data **data_thread, unsigned nb_data) {
    for (unsigned i = 0; i < nb_data; i++) {
        if (data_thread[i] != NULL) {
            free(data_thread[i]);
            data_thread[i] = NULL;
        }
    }
}

int init_tcp_threads_data(server_information *server, GAME_MODE mode, int game_id) {
    // TODO CHANGE FOR MULTIGAME
    unsigned *ready_player_number = malloc(sizeof(unsigned));
    *ready_player_number = 0;
    unsigned *connected_players = malloc(sizeof(unsigned));
    *connected_players = 0;
    bool *finished_flag = malloc(sizeof(bool));
    *finished_flag = false;
    unsigned *nb_players_left = malloc(sizeof(unsigned));
    *nb_players_left = 0;
    pthread_mutex_t *lock_waiting_all_players_join = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_t *lock_all_players_ready = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_t *lock_waiting_the_game_finish = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_t *lock_finished_flag = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_t *lock_nb_players_left = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_t *lock_all_tcp_threads_closed = malloc(sizeof(pthread_mutex_t));

    pthread_cond_t *cond_lock_waiting_all_players_join = malloc(sizeof(pthread_cond_t));
    pthread_cond_t *cond_lock_all_players_ready = malloc(sizeof(pthread_cond_t));
    pthread_cond_t *cond_lock_waiting_the_game_finish = malloc(sizeof(pthread_cond_t));
    pthread_cond_t *cond_lock_all_tcp_threads_closed = malloc(sizeof(pthread_cond_t));

    if (pthread_mutex_init(lock_waiting_the_game_finish, NULL) < 0) {
        return EXIT_FAILURE;
    }
    if (pthread_mutex_init(lock_all_players_ready, NULL) < 0) {
        return EXIT_FAILURE;
    }
    if (pthread_mutex_init(lock_waiting_all_players_join, NULL) < 0) {
        return EXIT_FAILURE;
    }
    if (pthread_mutex_init(lock_finished_flag, NULL) < 0) {
        return EXIT_FAILURE;
    }
    if (pthread_mutex_init(lock_nb_players_left, NULL) < 0) {
        return EXIT_FAILURE;
    }
    if (pthread_mutex_init(lock_all_tcp_threads_closed, NULL) < 0) {
        return EXIT_FAILURE;
    }
    if (pthread_cond_init(cond_lock_waiting_the_game_finish, NULL) < 0) {
        return EXIT_FAILURE;
    }
    if (pthread_cond_init(cond_lock_all_players_ready, NULL) < 0) {
        return EXIT_FAILURE;
    }
    if (pthread_cond_init(cond_lock_waiting_all_players_join, NULL) < 0) {
        return EXIT_FAILURE;
    }
    if (pthread_cond_init(cond_lock_all_tcp_threads_closed, NULL) < 0) {
        return EXIT_FAILURE;
    }

    for (unsigned i = 0; i < PLAYER_NUM; i++) {
        // TODO: refactor
        switch (mode) {
            case SOLO:
                solo_tcp_threads_data_players[i] = malloc(sizeof(tcp_thread_data));
                if (solo_tcp_threads_data_players[i] == NULL) {
                    perror("malloc tcp_thread_data");
                    for (unsigned j = 0; j < i; j++) {
                        free(solo_tcp_threads_data_players[j]);
                    }
                    return EXIT_FAILURE;
                }
                memset(solo_tcp_threads_data_players[i], 0, sizeof(tcp_thread_data));

                solo_tcp_threads_data_players[i]->ready_player_number = ready_player_number;
                solo_tcp_threads_data_players[i]->connected_players = connected_players;
                solo_tcp_threads_data_players[i]->finished_flag = finished_flag;
                solo_tcp_threads_data_players[i]->nb_players_left = nb_players_left;

                solo_tcp_threads_data_players[i]->lock_waiting_all_players_join = lock_waiting_all_players_join;
                solo_tcp_threads_data_players[i]->lock_all_players_ready = lock_all_players_ready;
                solo_tcp_threads_data_players[i]->lock_waiting_the_game_finish = lock_waiting_the_game_finish;
                solo_tcp_threads_data_players[i]->lock_finished_flag = lock_finished_flag;
                solo_tcp_threads_data_players[i]->lock_nb_players_left = lock_nb_players_left;
                solo_tcp_threads_data_players[i]->lock_all_tcp_threads_closed = lock_all_tcp_threads_closed;
                solo_tcp_threads_data_players[i]->cond_lock_waiting_all_players_join =
                    cond_lock_waiting_all_players_join;
                solo_tcp_threads_data_players[i]->cond_lock_all_players_ready = cond_lock_all_players_ready;
                solo_tcp_threads_data_players[i]->cond_lock_waiting_the_game_finish = cond_lock_waiting_the_game_finish;
                solo_tcp_threads_data_players[i]->cond_lock_all_tcp_threads_closed = cond_lock_all_tcp_threads_closed;

                solo_tcp_threads_data_players[i]->game_id = game_id;
                solo_tcp_threads_data_players[i]->eq = 0;
                solo_tcp_threads_data_players[i]->server = server;
                break;

            case TEAM:
                team_tcp_threads_data_players[i] = malloc(sizeof(tcp_thread_data));

                if (team_tcp_threads_data_players[i] == NULL) {
                    perror("malloc tcp_thread_data");
                    for (unsigned j = 0; j < i; j++) {
                        free(team_tcp_threads_data_players[j]);
                    }
                    return EXIT_FAILURE;
                }

                memset(team_tcp_threads_data_players[i], 0, sizeof(tcp_thread_data));

                team_tcp_threads_data_players[i]->ready_player_number = ready_player_number;
                team_tcp_threads_data_players[i]->connected_players = connected_players;
                team_tcp_threads_data_players[i]->finished_flag = finished_flag;
                team_tcp_threads_data_players[i]->nb_players_left = nb_players_left;

                team_tcp_threads_data_players[i]->lock_waiting_all_players_join = lock_waiting_all_players_join;
                team_tcp_threads_data_players[i]->lock_all_players_ready = lock_all_players_ready;
                team_tcp_threads_data_players[i]->lock_waiting_the_game_finish = lock_waiting_the_game_finish;
                team_tcp_threads_data_players[i]->lock_finished_flag = lock_finished_flag;
                team_tcp_threads_data_players[i]->lock_nb_players_left = lock_nb_players_left;
                team_tcp_threads_data_players[i]->lock_all_tcp_threads_closed = lock_all_tcp_threads_closed;
                team_tcp_threads_data_players[i]->cond_lock_waiting_all_players_join =
                    cond_lock_waiting_all_players_join;
                team_tcp_threads_data_players[i]->cond_lock_all_players_ready = cond_lock_all_players_ready;
                team_tcp_threads_data_players[i]->cond_lock_waiting_the_game_finish = cond_lock_waiting_the_game_finish;
                team_tcp_threads_data_players[i]->cond_lock_all_tcp_threads_closed = cond_lock_all_tcp_threads_closed;

                team_tcp_threads_data_players[i]->game_id = game_id;
                team_tcp_threads_data_players[i]->server = server;
                break;
            default:
                return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

void lock_mutex_to_wait(pthread_mutex_t *mutex, pthread_cond_t *cond) {
    pthread_mutex_lock(mutex);
    pthread_cond_wait(cond, mutex);
    pthread_mutex_unlock(mutex);
}

void unlock_mutex_for_everyone(pthread_mutex_t *mutex, pthread_cond_t *cond) {
    pthread_mutex_lock(mutex);
    pthread_cond_broadcast(cond);
    pthread_mutex_unlock(mutex);
}

initial_connection_header *recv_initial_connection_header_of_client(int sock) {
    return recv_initial_connection_header(sock);
}

ready_connection_header *recv_ready_connexion_header_of_client(int sock) {
    return recv_ready_connexion_header(sock);
}

game_action *recv_game_action_of_clients(server_information *server) {
    return recv_game_action(server->sock_udp);
}

chat_message *recv_chat_message_of_client(server_information *server, int id) {
    if (server->sock_clients[id] == -1) {
        return NULL;
    }

    return recv_chat_message(server->sock_clients[id]);
}

int send_connexion_information_of_client(server_information *server, int id, int eq) {
    // TODO Replace the gamemode
    return send_connexion_information(server->sock_clients[id], SOLO, id, eq, ntohs(server->port_udp),
                                      ntohs(server->port_mult), server->adrmdiff);
}

int send_game_board_for_clients(server_information *server, uint16_t num, board *board_) {
    return send_game_board(server->sock_mult, server->addr_mult, num, board_);
}

int send_game_update_for_clients(server_information *server, uint16_t num, tile_diff *diff, uint8_t nb) {
    return send_game_update(server->sock_mult, server->addr_mult, num, diff, nb);
}

int send_chat_message_to_client(server_information *server, int id, chat_message_type type, int sender_id, int eq,
                                uint8_t message_length, char *message) {
    return send_chat_message(server->sock_clients[id], type, sender_id, eq, message_length, message);
}

void handle_chat_message_global(server_information *server, int sender_id, chat_message *msg) {
    for (int i = 0; i < PLAYER_NUM; i++) {
        if (i == sender_id) {
            continue; // Don't send the message to the sender
        }
        if (server->sock_clients[i] == -1) {
            continue; // Don't send the message to the client if it is not connected
        }

        if (send_chat_message_to_client(server, i, msg->type, sender_id, msg->eq, msg->message_length, msg->message) <
            0) {
            perror("send_chat_message_to_client");
        }
    }
}

void handle_chat_message_team(server_information *server, int sender_id, chat_message *msg) {
    for (int i = 0; i < PLAYER_NUM; i++) {
        if (i == sender_id) {
            continue; // Don't send the message to the sender or to the other team
        }

        if ((sender_id == 0 || sender_id == 3) && (i == 1 || i == 2)) {
            continue; // If the sender is in the first team, don't send the message to the second team
        }

        if ((sender_id == 1 || sender_id == 2) && (i == 0 || i == 3)) {
            continue; // If the sender is in the second team, don't send the message to the first team
        }

        if (server->sock_clients[i] == -1) {
            continue; // Don't send the message to the client if it is not connected
        }

        if (send_chat_message_to_client(server, i, msg->type, sender_id, msg->eq, msg->message_length, msg->message) <
            0) {
            perror("send_chat_message_to_client");
        }
    }
}

void handle_chat_message(server_information *server, int sender_id, chat_message *msg) {
    if (get_game_mode(TMP_GAME_ID) == SOLO) {
        handle_chat_message_global(server, sender_id, msg);
    } else if (get_game_mode(TMP_GAME_ID) == TEAM) {
        if (msg->type == GLOBAL_M) {
            handle_chat_message_global(server, sender_id, msg);
        } else if (msg->type == TEAM_M) {
            handle_chat_message_team(server, sender_id, msg);
        }
    } else {
        perror("Unknown game mode");
    }
}

void handle_game_over(server_information *server, int game_id) {
    GAME_MODE mode;
    pthread_mutex_lock(lock_game_model);
    mode = get_game_mode(game_id);
    pthread_mutex_unlock(lock_game_model);

    if (mode == SOLO) {
        pthread_mutex_lock(lock_game_model);
        int winner_player = get_winner_solo(game_id);
        pthread_mutex_unlock(lock_game_model);
        for (int i = 0; i < PLAYER_NUM; i++) {
            if (server->sock_clients[i] != -1) {
                if (send_game_over(server->sock_clients[i], SOLO, winner_player, 0) < 0) {
                    perror("send_game_over");
                }
            }
        }
    } else if (mode == TEAM) {
        pthread_mutex_lock(lock_game_model);
        int winner_team = get_winner_team(game_id);
        pthread_mutex_unlock(lock_game_model);
        for (int i = 0; i < PLAYER_NUM; i++) {
            if (server->sock_clients[i] != -1) {
                if (send_game_over(server->sock_clients[i], TEAM, 0, winner_team) < 0) {
                    perror("send_game_over");
                }
            }
        }
    } else {
        perror("Unknown game mode");
    }
}

struct pollfd *init_polls_connexion(int sock) {
    struct pollfd *polls = malloc(INITIAL_POLL_FD_SIZE * sizeof(struct pollfd));
    RETURN_NULL_IF_NULL_PERROR(polls, "malloc polls");
    polls[0].fd = sock;
    polls[0].events = POLL_IN;
    return polls;
}

struct pollfd *add_polls_to_poll(struct pollfd *polls, unsigned *nb, unsigned *s, int sock) {
    if (*nb == *s) {
        struct pollfd *n = (struct pollfd *)realloc(polls, 4 * (*s) * sizeof(struct pollfd));
        *s *= 4;
        *nb += 1;
        return n;
    } else {
        polls[*nb].fd = sock;
        polls[*nb].events = POLL_IN;
        *nb += 1;
        return polls;
    }
}

void remove_polls_to_poll(struct pollfd *polls, unsigned *nb, unsigned i) {
    if (i >= *nb) {
        return;
    }
    polls[i].fd = polls[0].fd;
    *nb -= 1;
}

void print_ready_player(ready_connection_header *ready_informations) {
    printf("Player with Id : %d, eq : %d is ready.\n", ready_informations->id, ready_informations->eq);
}

int try_to_init_socket_of_client() {
    struct sockaddr_in6 client_addr;
    int client_addr_len = sizeof(client_addr);
    int res = accept(sock_tcp, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
    if (res < 0) {
        perror("client acceptance");
        return EXIT_FAILURE;
    }

    print_ip_of_client(client_addr);
    return res;
}

int init_game_model(GAME_MODE mode) {
    dimension dim;
    dim.width = GAMEBOARD_WIDTH;
    dim.height = GAMEBOARD_HEIGHT;

    printf("lock init game_model\n");
    int game_id = init_model(dim, mode);

    return game_id;
}

int add_game_action_to_thread_data(udp_thread_data *data, game_action *action) {
    if (data->size_game_actions == 0) {
        data->size_game_actions = INITIAL_GAME_ACTIONS_SIZE;
        data->game_actions = malloc(sizeof(game_action *) * data->size_game_actions);
    }
    if (data->nb_game_actions + 1 >= data->size_game_actions) {
        data->game_actions = realloc(data->game_actions, data->size_game_actions * sizeof(game_action *) * 2);
        RETURN_FAILURE_IF_NULL(data->game_actions);
        data->size_game_actions *= 2;
    }
    data->game_actions[data->nb_game_actions] = action;
    data->nb_game_actions++;
    return EXIT_SUCCESS;
}

int empty_game_actions(udp_thread_data *data) {
    for (unsigned i = 0; i < data->nb_game_actions; i++) {
        free(data->game_actions[i]);
        data->game_actions[i] = NULL;
    }
    data->nb_game_actions = 0;
    return EXIT_SUCCESS;
}

void free_game_actions(game_action **game_actions, size_t nb_game_actions) {
    for (unsigned i = 0; i < nb_game_actions; i++) {
        free(game_actions[i]);
    }
    free(game_actions);
}

void *serv_client_recv_game_action(void *arg_udp_thread_data) {
    udp_thread_data *data = (udp_thread_data *)arg_udp_thread_data;
    while (true) {
        pthread_mutex_lock(data->lock_finished_flag);
        if (*data->finished_flag) {
            pthread_mutex_unlock(data->lock_finished_flag);
            break;
        }
        pthread_mutex_unlock(data->lock_finished_flag);

        game_action *action = recv_game_action_of_clients(data->server);
        pthread_mutex_lock(lock_game_model);
        GAME_MODE game_mode = get_game_mode(data->game_id);
        pthread_mutex_unlock(lock_game_model);
        if (action != NULL && action->game_mode == game_mode) {
            pthread_mutex_lock(&data->lock_game_actions);
            add_game_action_to_thread_data(data, action);
            pthread_mutex_unlock(&data->lock_game_actions);
        }
    }

    sleep(2); // Wait for the other thread to finish

    pthread_mutex_lock(data->lock_nb_stopped_udp_threads);
    *data->nb_stopped_udp_threads += 1;

    if (*data->nb_stopped_udp_threads == 2) {
        unlock_mutex_for_everyone(data->lock_all_udp_threads_closed, data->cond_lock_all_udp_threads_closed);
    }

    pthread_mutex_unlock(data->lock_nb_stopped_udp_threads);

    printf("Recv game action thread finished\n");

    return NULL;
}

void increment_last_num_message(int *last_num_message) {
    *last_num_message = *last_num_message + 1 % LIMIT_LAST_NUM_MESSAGE_MULT;
}

void *serve_clients_send_mult_sec(void *arg_udp_thread_data) {
    udp_thread_data *data = (udp_thread_data *)arg_udp_thread_data;

    int last_num_sec_message = 1;
    while (true) {
        sleep(1);
        pthread_mutex_lock(lock_game_model);
        update_bombs(data->game_id);
        pthread_mutex_unlock(lock_game_model);

        pthread_mutex_lock(lock_game_model);
        board *game_board = get_game_board(data->game_id);
        pthread_mutex_unlock(lock_game_model);
        RETURN_NULL_IF_NULL(game_board);

        pthread_mutex_lock(&data->lock_send_udp);
        send_game_board_for_clients(data->server, last_num_sec_message, game_board);
        pthread_mutex_unlock(&data->lock_send_udp);
        increment_last_num_message(&last_num_sec_message);
        free(game_board);
        // TODO manage errors

        pthread_mutex_lock(lock_game_model);
        bool game_over = is_game_over(data->game_id);
        pthread_mutex_unlock(lock_game_model);

        if (game_over) {
            pthread_mutex_lock(data->lock_finished_flag);
            *data->finished_flag = true;
            pthread_mutex_unlock(data->lock_finished_flag);

            printf("Mult sec thread finished 1\n");

            // Wait for the TCP threads to finish
            lock_mutex_to_wait(data->lock_all_tcp_threads_closed, data->cond_lock_all_tcp_threads_closed);

            printf("Mult sec thread finished 2\n");

            // Wait for the UDP threads to finish
            lock_mutex_to_wait(data->lock_all_udp_threads_closed, data->cond_lock_all_udp_threads_closed);

            printf("Mult sec thread finished 3\n");

            pthread_mutex_lock(&data->lock_game_actions);
            free_game_actions(data->game_actions, data->nb_game_actions);
            pthread_mutex_unlock(&data->lock_game_actions);

            pthread_mutex_lock(lock_game_model);
            remove_game(data->game_id);
            pthread_mutex_unlock(lock_game_model);

            pthread_mutex_destroy(data->lock_finished_flag);
            free(data->lock_finished_flag);

            pthread_mutex_destroy(&data->lock_send_udp);

            pthread_mutex_destroy(&data->lock_game_actions);

            pthread_mutex_destroy(data->lock_nb_stopped_udp_threads);
            free(data->lock_nb_stopped_udp_threads);

            pthread_mutex_destroy(data->lock_all_tcp_threads_closed);
            free(data->lock_all_tcp_threads_closed);

            pthread_cond_destroy(data->cond_lock_all_tcp_threads_closed);
            free(data->cond_lock_all_tcp_threads_closed);

            pthread_mutex_destroy(data->lock_all_udp_threads_closed);
            free(data->lock_all_udp_threads_closed);

            pthread_cond_destroy(data->cond_lock_all_udp_threads_closed);
            free(data->cond_lock_all_udp_threads_closed);

            break;
        }
    }
    free(data);

    printf("Mult sec thread finished 4\n");
    return NULL;
}

/** The message is considered as a next message if it is between the last message
 * and a fairly large part after the latter (half the limit) modulo limit
 */
bool is_next_message(int last_num_message, int num_message, int limit) {
    int just_after = (last_num_message + 1) % limit;
    int limit_next_message = (last_num_message + 1 + (limit / 2)) % limit;

    if (just_after < limit_next_message) {
        return num_message >= just_after && num_message < limit_next_message;
    } else {
        return (num_message >= just_after && num_message < limit) || (num_message <= limit_next_message);
    }
}

void game_action_swap(game_action **game_actions, unsigned i, unsigned j) {
    game_action *current_action = game_actions[j];
    game_actions[j] = game_actions[i];
    game_actions[i] = current_action;
}

unsigned game_action_partition(game_action **game_actions, int start, int end, int last_num_message[PLAYER_NUM]) {
    game_action *pivot = game_actions[end];
    unsigned j = start;

    for (int i = start; i < end - 1; i++) {
        if (game_actions[i]->id != pivot->id) {
            continue;
        }
        if (!is_next_message(last_num_message[pivot->id], pivot->message_number, LIMIT_LAST_NUM_MESSAGE_CLIENT) &&
            is_next_message(last_num_message[game_actions[i]->id], game_actions[i]->message_number,
                            LIMIT_LAST_NUM_MESSAGE_CLIENT)) {
            continue;
        }
        if (abs(last_num_message[pivot->id] - pivot->message_number) <=
            abs(last_num_message[game_actions[i]->id] - game_actions[i]->message_number)) {
            continue;
        }
        game_action_swap(game_actions, i, j);
        j++;
    }
    game_action_swap(game_actions, j, end);

    return j;
}

void game_action_quick_sort(game_action **game_actions, int start, int end, int last_num_message[PLAYER_NUM]) {
    if (start >= end) {
        return;
    }
    unsigned pivot = game_action_partition(game_actions, start, end, last_num_message);

    game_action_quick_sort(game_actions, start, pivot - 1, last_num_message);
    game_action_quick_sort(game_actions, pivot + 1, end, last_num_message);
}

void game_actions_sort(game_action **game_actions, size_t nb_game_actions, int last_num_message[PLAYER_NUM]) {
    game_action_quick_sort(game_actions, 0, nb_game_actions - 1, last_num_message);
}

player_action *merge_player_moves_and_place_bomb(player_action *player_moves, unsigned nb_player_moves,
                                                 player_action *player_place_bomb, unsigned nb_place_bomb,
                                                 unsigned *nb_player_actions) {
    if (nb_player_actions == 0 && nb_place_bomb == 0) {
        return NULL;
    }
    player_action *res = malloc(sizeof(player_action) * (nb_player_moves + nb_place_bomb));
    RETURN_NULL_IF_NULL_PERROR(res, "malloc player_action");

    memcpy(res, player_moves, sizeof(player_action) * nb_player_moves);
    memcpy(res + sizeof(player_action) * nb_player_moves, player_place_bomb, sizeof(player_action) * nb_place_bomb);

    *nb_player_actions = nb_player_moves + nb_place_bomb;
    return res;
}

player_action *get_player_actions(game_action **game_actions, size_t nb_game_actions,
                                  int last_num_received_message[PLAYER_NUM], unsigned *nb_player_actions) {
    if (game_actions == NULL) {
        return NULL;
    }
    player_action *player_moves = malloc(sizeof(player_action) * PLAYER_NUM);
    RETURN_NULL_IF_NULL_PERROR(player_moves, "malloc player_moves");
    player_action *player_place_bomb = malloc(sizeof(player_action) * PLAYER_NUM);
    RETURN_NULL_IF_NULL_PERROR(player_place_bomb, "malloc player_place_bomb");

    unsigned nb_player_moves = 0;
    unsigned nb_place_bomb = 0;

    bool already_move[PLAYER_NUM];
    bool already_place_bomb[PLAYER_NUM];
    for (unsigned i = 0; i < PLAYER_NUM; i++) {
        already_move[i] = false;
        already_place_bomb[i] = false;
    }

    for (int i = nb_game_actions - 1; i >= 0; i--) {
        // Other actions were sent before those kept
        if (nb_player_moves == PLAYER_NUM && nb_place_bomb == PLAYER_NUM) {
            break;
        }
        // Message ignored
        if (!is_next_message(last_num_received_message[game_actions[i]->id], game_actions[i]->message_number,
                             LIMIT_LAST_NUM_MESSAGE_CLIENT)) {
            continue;
        }
        // Keep the action if its a move and there is no move kept for this player
        if (is_move(game_actions[i]->action) && !already_move[game_actions[i]->id]) {
            player_moves[nb_player_moves].id = game_actions[i]->id;
            player_moves[nb_player_moves].action = game_actions[i]->action;
            already_move[game_actions[i]->id] = true;

            // To keep the last message number
            if (!already_place_bomb[game_actions[i]->id]) {
                last_num_received_message[game_actions[i]->id] = game_actions[i]->message_number;
            }
            nb_player_moves++;
            continue;
        }
        // Keep the action if its a place bomb and there is no bomb placing move kept for this player
        if (game_actions[i]->action == GAME_PLACE_BOMB && !already_place_bomb[game_actions[i]->id]) {
            player_place_bomb[nb_place_bomb].id = game_actions[i]->id;
            player_place_bomb[nb_place_bomb].action = game_actions[i]->action;
            already_place_bomb[game_actions[i]->id] = true;

            // To keep the last message number
            if (!already_move[game_actions[i]->id]) {
                last_num_received_message[game_actions[i]->id] = game_actions[i]->message_number;
            }
            nb_place_bomb++;
        }
    }

    if (nb_player_moves == 0 && nb_place_bomb == 0) {
        return NULL;
    }

    if (nb_player_moves == 0) {
        free(player_moves);
        *nb_player_actions = nb_place_bomb;
        return player_place_bomb;
    }

    if (nb_place_bomb == 0) {
        free(player_place_bomb);
        *nb_player_actions = nb_player_moves;
        return player_moves;
    }

    player_action *res = merge_player_moves_and_place_bomb(player_moves, nb_player_moves, player_place_bomb,
                                                           nb_place_bomb, nb_player_actions);
    free(player_moves);
    free(player_place_bomb);
    return res;
}

game_action **copy_game_actions(game_action **game_actions, size_t nb_game_actions) {
    if (nb_game_actions == 0) {
        return NULL;
    }
    game_action **res = malloc(nb_game_actions * (sizeof(game_action *)));
    RETURN_NULL_IF_NULL(res);

    for (unsigned i = 0; i < nb_game_actions; i++) {
        res[i] = malloc(sizeof(game_action));
        memcpy(res[i], game_actions[i], sizeof(game_action));
    }
    return res;
}

void *serve_clients_send_mult_freq(void *arg_udp_thread_data) {
    udp_thread_data *data = (udp_thread_data *)arg_udp_thread_data;
    int last_num_received_messages[PLAYER_NUM];
    for (unsigned i = 0; i < PLAYER_NUM; i++) {
        last_num_received_messages[i] = LIMIT_LAST_NUM_MESSAGE_CLIENT - 1;
    }

    int last_num_freq_message = 0;
    while (true) {
        usleep(FREQ);

        // Check if the game is over
        pthread_mutex_lock(data->lock_finished_flag);
        if (*data->finished_flag) {
            pthread_mutex_unlock(data->lock_finished_flag);
            break;
        }
        pthread_mutex_unlock(data->lock_finished_flag);

        // Copy game actions
        pthread_mutex_lock(&data->lock_game_actions);
        size_t nb_game_actions = data->nb_game_actions;
        if (nb_game_actions == 0) {
            pthread_mutex_unlock(&data->lock_game_actions);
            continue;
        }
        game_action **game_actions = copy_game_actions(data->game_actions, nb_game_actions);
        RETURN_NULL_IF_NULL(game_actions);
        empty_game_actions(data);
        pthread_mutex_unlock(&data->lock_game_actions);

        game_actions_sort(game_actions, data->nb_game_actions, last_num_received_messages);

        // Get player actions
        unsigned nb_player_actions;
        player_action *player_actions =
            get_player_actions(game_actions, nb_game_actions, last_num_received_messages, &nb_player_actions);
        if (player_actions == NULL) {
            continue;
        }
        // Update the board with player actions and get the tile differences
        unsigned size_tile_diff = 0;

        pthread_mutex_lock(lock_game_model);
        tile_diff *diffs = update_game_board(data->game_id, player_actions, nb_player_actions, &size_tile_diff);
        pthread_mutex_unlock(lock_game_model);
        RETURN_NULL_IF_NULL(diffs);

        if (size_tile_diff == 0) {
            continue;
        }

        // Send the differences
        pthread_mutex_lock(&data->lock_send_udp);
        send_game_update_for_clients(data->server, last_num_freq_message, diffs, size_tile_diff);
        pthread_mutex_unlock(&data->lock_send_udp);

        // Last free
        free(diffs);
        free_game_actions(game_actions, nb_game_actions);

        // Prepare new message
        increment_last_num_message(&last_num_freq_message);
    }

    sleep(2); // Wait for the other thread to finish

    pthread_mutex_lock(data->lock_nb_stopped_udp_threads);
    *data->nb_stopped_udp_threads += 1;

    if (*data->nb_stopped_udp_threads == 2) {

        unlock_mutex_for_everyone(data->lock_all_udp_threads_closed, data->cond_lock_all_udp_threads_closed);
    }

    pthread_mutex_unlock(data->lock_nb_stopped_udp_threads);

    printf("Mult freq thread finished\n");

    return NULL;
}

int init_game_threads(server_information *server, bool *finished_flag, pthread_mutex_t *lock_finished_flag,
                      pthread_mutex_t *lock_all_tcp_threads_closed, pthread_cond_t *cond_lock_all_tcp_threads_closed,
                      unsigned game_id) {
    udp_thread_data *udp_thread_data_game = malloc(sizeof(udp_thread_data));
    RETURN_FAILURE_IF_NULL(udp_thread_data_game);
    udp_thread_data_game->finished_flag = finished_flag;
    udp_thread_data_game->game_id = game_id;
    udp_thread_data_game->game_actions = NULL;
    udp_thread_data_game->size_game_actions = 0;
    udp_thread_data_game->nb_game_actions = 0;
    udp_thread_data_game->nb_stopped_udp_threads = malloc(sizeof(unsigned));
    RETURN_FAILURE_IF_NULL(udp_thread_data_game->nb_stopped_udp_threads);
    *udp_thread_data_game->nb_stopped_udp_threads = 0;

    udp_thread_data_game->server = server;

    udp_thread_data_game->lock_finished_flag = lock_finished_flag;
    udp_thread_data_game->lock_all_tcp_threads_closed = lock_all_tcp_threads_closed;
    udp_thread_data_game->lock_nb_stopped_udp_threads = malloc(sizeof(pthread_mutex_t));
    RETURN_FAILURE_IF_NULL(udp_thread_data_game->lock_nb_stopped_udp_threads);
    udp_thread_data_game->lock_all_udp_threads_closed = malloc(sizeof(pthread_mutex_t));
    RETURN_FAILURE_IF_NULL(udp_thread_data_game->lock_all_udp_threads_closed);
    udp_thread_data_game->cond_lock_all_tcp_threads_closed = cond_lock_all_tcp_threads_closed;
    udp_thread_data_game->cond_lock_all_udp_threads_closed = malloc(sizeof(pthread_cond_t));
    RETURN_FAILURE_IF_NULL(udp_thread_data_game->cond_lock_all_udp_threads_closed);

    if (pthread_mutex_init(&udp_thread_data_game->lock_game_actions, NULL) < 0) {
        goto EXIT_FREEING_DATA;
    }
    if (pthread_mutex_init(&udp_thread_data_game->lock_send_udp, NULL) < 0) {
        goto EXIT_FREEING_DATA;
    }
    if (pthread_mutex_init(udp_thread_data_game->lock_nb_stopped_udp_threads, NULL) < 0) {
        goto EXIT_FREEING_DATA;
    }
    if (pthread_mutex_init(udp_thread_data_game->lock_all_udp_threads_closed, NULL) < 0) {
        goto EXIT_FREEING_DATA;
    }
    if (pthread_cond_init(udp_thread_data_game->cond_lock_all_udp_threads_closed, NULL) < 0) {
        goto EXIT_FREEING_DATA;
    }

    if (pthread_create(&game_threads[0], NULL, serv_client_recv_game_action, udp_thread_data_game) < 0) {
        goto EXIT_FREEING_DATA;
    }
    if (pthread_create(&game_threads[1], NULL, serve_clients_send_mult_sec, udp_thread_data_game) < 0) {
        goto EXIT_FREEING_DATA;
    }
    if (pthread_create(&game_threads[2], NULL, serve_clients_send_mult_freq, udp_thread_data_game) < 0) {
        goto EXIT_FREEING_DATA;
    }

    return EXIT_SUCCESS;

EXIT_FREEING_DATA:
    free(udp_thread_data_game);
    return EXIT_FAILURE;
}

void wait_all_clients_connected(pthread_mutex_t *lock, pthread_cond_t *cond, bool is_everyone_connected,
                                unsigned *nb_connected) {
    if (!is_everyone_connected) {
        pthread_cond_wait(cond, lock);
    } else {
        free(nb_connected);
        pthread_cond_broadcast(cond);
    }
    pthread_mutex_unlock(lock);
}

void wait_all_clients_not_ready(server_information *server, bool *finished_flag, pthread_mutex_t *lock_finished_flag,
                                pthread_mutex_t *lock_all_tcp_threads_closed,
                                pthread_cond_t *cond_lock_all_tcp_threads_closed, pthread_mutex_t *lock,
                                pthread_cond_t *cond, bool is_ready, unsigned *nb_ready, int game_id) {
    if (!is_ready) {
        pthread_cond_wait(cond, lock);
    } else {
        printf("lock wait all clients\n");
        pthread_mutex_lock(lock_game_model);
        board *game_board = get_game_board(game_id);
        pthread_mutex_unlock(lock_game_model);
        send_game_board_for_clients(server, 0, game_board); // Initial game_board send
        free_board(game_board);
        init_game_threads(server, finished_flag, lock_finished_flag, lock_all_tcp_threads_closed,
                          cond_lock_all_tcp_threads_closed, game_id); // TODO MANAGE ERRORS

        free(nb_ready);
        pthread_cond_broadcast(cond);
    }
    pthread_mutex_unlock(lock);
}

void handle_tcp_communication(tcp_thread_data *tcp_data) {
    int client_sock = tcp_data->server->sock_clients[tcp_data->id];
    char buffer[1];
    fd_set read_fds;
    struct timeval tv;
    int retval;

    while (true) {
        pthread_mutex_lock(tcp_data->lock_finished_flag);
        if (*tcp_data->finished_flag) {
            pthread_mutex_unlock(tcp_data->lock_finished_flag);
            break;
        }
        pthread_mutex_unlock(tcp_data->lock_finished_flag);

        if (recv(client_sock, buffer, 1, MSG_PEEK | MSG_DONTWAIT) == 0) {
            printf("Player %d closed read\n", tcp_data->id);
            pthread_mutex_lock(lock_game_model);
            set_player_dead(tcp_data->game_id, tcp_data->id);
            pthread_mutex_unlock(lock_game_model);
            break;
        }

        FD_ZERO(&read_fds);
        FD_SET(client_sock, &read_fds);

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        retval = select(client_sock + 1, &read_fds, NULL, NULL, &tv);

        if (retval == -1) {
            perror("select");
            break;
        } else if (retval > 0) {
            if (FD_ISSET(client_sock, &read_fds)) {
                chat_message *msg = recv_chat_message_of_client(tcp_data->server, tcp_data->id);

                if (msg != NULL) {
                    // Check if sending a message to the client is possible
                    if (send(client_sock, buffer, 0, MSG_NOSIGNAL) < 0) {
                        printf("Player %d closed read\n", tcp_data->id);
                        pthread_mutex_lock(lock_game_model);
                        set_player_dead(tcp_data->game_id, tcp_data->id);
                        pthread_mutex_unlock(lock_game_model);
                        break;
                    }

                    handle_chat_message(tcp_data->server, tcp_data->id, msg);
                    free(msg->message);
                    free(msg);
                } else {
                    printf("Player %d closed write\n", tcp_data->id);
                    pthread_mutex_lock(lock_game_model);
                    set_player_dead(tcp_data->game_id, tcp_data->id);
                    pthread_mutex_unlock(lock_game_model);
                    break;
                }
            }
        }
    }

    pthread_mutex_lock(tcp_data->lock_nb_players_left);
    if (*tcp_data->nb_players_left == PLAYER_NUM - 1) {

        handle_game_over(tcp_data->server, tcp_data->game_id);

        // Last player left
        if (tcp_data->nb_players_left != NULL) {
            free(tcp_data->nb_players_left);
            tcp_data->nb_players_left = NULL;
        }

        if (tcp_data->lock_waiting_all_players_join != NULL) {
            pthread_mutex_unlock(tcp_data->lock_waiting_all_players_join);
            pthread_mutex_destroy(tcp_data->lock_waiting_all_players_join);
            free(tcp_data->lock_waiting_all_players_join);
            tcp_data->lock_waiting_all_players_join = NULL;
        }

        if (tcp_data->lock_all_players_ready != NULL) {
            pthread_mutex_unlock(tcp_data->lock_all_players_ready);
            pthread_mutex_destroy(tcp_data->lock_all_players_ready);
            free(tcp_data->lock_all_players_ready);
            tcp_data->lock_all_players_ready = NULL;
        }

        if (tcp_data->lock_waiting_the_game_finish != NULL) {
            pthread_mutex_unlock(tcp_data->lock_waiting_the_game_finish);
            pthread_mutex_destroy(tcp_data->lock_waiting_the_game_finish);
            free(tcp_data->lock_waiting_the_game_finish);
            tcp_data->lock_waiting_the_game_finish = NULL;
        }

        if (tcp_data->lock_nb_players_left != NULL) {
            pthread_mutex_unlock(tcp_data->lock_nb_players_left);
            pthread_mutex_destroy(tcp_data->lock_nb_players_left);
            free(tcp_data->lock_nb_players_left);
            tcp_data->lock_nb_players_left = NULL;
        }

        // Close all sockets
        for (int i = 0; i < PLAYER_NUM; i++) {
            close_socket_client(tcp_data->server, i);
        }

        close_socket_udp(tcp_data->server);
        close_socket_mult(tcp_data->server);

        unlock_mutex_for_everyone(tcp_data->lock_all_tcp_threads_closed, tcp_data->cond_lock_all_tcp_threads_closed);

        return;
    } else {
        (*tcp_data->nb_players_left)++;
    }
    pthread_mutex_unlock(tcp_data->lock_nb_players_left);
}

void *serve_client_tcp(void *arg_tcp_thread_data) {
    tcp_thread_data *tcp_data = (tcp_thread_data *)arg_tcp_thread_data;
    pthread_mutex_lock(tcp_data->lock_waiting_all_players_join);
    *(tcp_data->connected_players) += 1;
    printf("%d\n", *tcp_data->connected_players);
    bool is_everyone_connected = *tcp_data->connected_players == PLAYER_NUM;
    wait_all_clients_connected(tcp_data->lock_waiting_all_players_join, tcp_data->cond_lock_waiting_all_players_join,
                               is_everyone_connected, tcp_data->connected_players);
    printf("%d send\n", tcp_data->id);
    send_connexion_information_of_client(tcp_data->server, tcp_data->id, tcp_data->eq);
    printf("%d after send\n", tcp_data->id);

    // TODO verify ready_informations
    ready_connection_header *ready_informations =
        recv_ready_connexion_header_of_client(tcp_data->server->sock_clients[tcp_data->id]);
    print_ready_player(ready_informations);
    pthread_mutex_lock(tcp_data->lock_all_players_ready);
    *(tcp_data->ready_player_number) += 1;
    bool is_ready = *tcp_data->ready_player_number == PLAYER_NUM;
    wait_all_clients_not_ready(tcp_data->server, tcp_data->finished_flag, tcp_data->lock_finished_flag,
                               tcp_data->lock_all_tcp_threads_closed, tcp_data->cond_lock_all_tcp_threads_closed,
                               tcp_data->lock_all_players_ready, tcp_data->cond_lock_all_players_ready, is_ready,
                               tcp_data->ready_player_number, tcp_data->game_id);

    handle_tcp_communication(tcp_data);

    printf("Player %d left the game.\n", ready_informations->id);
    free(ready_informations);

    printf("TCP thread finished : %d\n", tcp_data->id);

    free(tcp_data);

    sleep(3); // Just in case

    return NULL;
}

int connect_one_player_to_game(int sock) {
    // TODO: Init servers
    // TODO change recv not tcp_data
    initial_connection_header *head = recv_initial_connection_header_of_client(sock);

    // TODO: refactor
    if (head->game_mode == SOLO) {
        if (connected_solo_players == 0) {
            pthread_mutex_lock(lock_game_model);
            int game_id = init_game_model(SOLO);
            pthread_mutex_unlock(lock_game_model);
            if (game_id == -1) {
                return EXIT_FAILURE;
            }
            solo_waiting_server = init_server_network(connection_port);
            RETURN_FAILURE_IF_NULL(solo_waiting_server);
            init_tcp_threads_data(solo_waiting_server, SOLO, game_id);
        }
        solo_waiting_server->sock_clients[connected_solo_players] = sock;
        solo_tcp_threads_data_players[connected_solo_players]->id = connected_solo_players;
        *(solo_tcp_threads_data_players[connected_solo_players]->connected_players) = connected_solo_players;
        connected_solo_players++;

        pthread_t t;
        if (pthread_create(&t, NULL, serve_client_tcp, solo_tcp_threads_data_players[connected_solo_players - 1]) < 0) {
            perror("thread creation");
            return EXIT_FAILURE;
        }

    } else if (head->game_mode == TEAM) {
        if (connected_team_players == 0) {
            pthread_mutex_lock(lock_game_model);
            int game_id = init_game_model(TEAM);
            pthread_mutex_unlock(lock_game_model);
            if (game_id == -1) {
                return EXIT_FAILURE;
            }
            team_waiting_server = init_server_network(connection_port);
            RETURN_FAILURE_IF_NULL(team_waiting_server);
            if (game_id == 0 || game_id == 3) { // 0 and 3 are in the same team
                init_tcp_threads_data(team_waiting_server, TEAM, 0);
            } else {
                init_tcp_threads_data(team_waiting_server, TEAM, 1);
            }
        }
        team_waiting_server->sock_clients[connected_team_players] = sock;
        team_tcp_threads_data_players[connected_team_players]->id = connected_team_players;

        if (connected_team_players == 0 || connected_team_players == 3) {
            team_tcp_threads_data_players[connected_team_players]->eq = 0;
        } else {
            team_tcp_threads_data_players[connected_team_players]->eq = 1;
        }

        *(team_tcp_threads_data_players[connected_team_players]->connected_players) = connected_team_players;
        connected_team_players++;

        pthread_t t;
        if (pthread_create(&t, NULL, serve_client_tcp, team_tcp_threads_data_players[connected_team_players - 1]) < 0) {
            perror("thread creation");
            return EXIT_FAILURE;
        }
    }
    free(head);

    connected_solo_players %= PLAYER_NUM;
    connected_team_players %= PLAYER_NUM;

    return EXIT_SUCCESS;
}

int connect_players_to_game() {
    listen_players();
    printf("Waiting players on %u port.\n", get_port_tcp());

    struct pollfd *polls = init_polls_connexion(sock_tcp);
    unsigned nbfds = 1;
    unsigned s = INITIAL_POLL_FD_SIZE;

    while (1) {
        poll(polls, nbfds, -1);

        for (unsigned i = 1; i < nbfds; i++) {
            if (polls[i].revents & POLL_IN) {
                connect_one_player_to_game(polls[i].fd);
                remove_polls_to_poll(polls, &nbfds, i);
            }
            if (polls[i].revents & POLL_HUP) {
                close(polls[i].fd);
                remove_polls_to_poll(polls, &nbfds, i);
            }
        }
        if (polls[0].revents & POLL_IN) {
            int sock = try_to_init_socket_of_client();
            add_polls_to_poll(polls, &nbfds, &s, sock);
        }
    }

    printf("All players are connected.\n");
    return EXIT_SUCCESS;
}

int game_loop_server() {
    int return_value;

    lock_game_model = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(lock_game_model, NULL);

    if (connect_players_to_game() != EXIT_SUCCESS) {
        return_value = EXIT_FAILURE;
        goto exit_closing_sockets_and_free_addr_mult;
    }
    sleep(1); // Wait all clients for the join mutex
    goto exit_closing_sockets_and_free_addr_mult;

exit_closing_sockets_and_free_addr_mult:
    close_socket_tcp();
    free(lock_game_model);
    return return_value;
}
