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
#include <unistd.h>

#define MAX_PORT_TRY 250
#define START_ADRMDIFF 0xff12

#define ERROR_ADDRINUSE 2

#define LIMIT_LAST_NUM_MESSAGE_MULT ((1 << 15) - 1)   // 2^16
#define LIMIT_LAST_NUM_MESSAGE_CLIENT ((1 << 12) - 1) // 2^13

#define TMP_GAME_ID 0
#define FREQ 50000 // 100 00 us = 10 ms
#define INITIAL_GAME_ACTIONS_SIZE 4
#define INITIAL_POLL_FD_SIZE 9

typedef struct tcp_thread_data {
    unsigned id;
    unsigned *ready_player_number;
    unsigned *connected_players;
    pthread_mutex_t *lock_waiting_all_players_join;
    pthread_mutex_t *lock_all_players_ready;
    pthread_mutex_t *lock_waiting_the_game_finish;
    pthread_cond_t *cond_lock_waiting_all_players_join;
    pthread_cond_t *cond_lock_all_players_ready;
    pthread_cond_t *cond_lock_waiting_the_game_finish;
    bool *finished_flag;
} tcp_thread_data;

typedef struct udp_thread_data {
    unsigned game_id;
    game_action **game_actions;
    unsigned nb_game_actions;
    unsigned size_game_actions;
    bool finished_flag;
    pthread_mutex_t lock_game_actions;
    pthread_mutex_t lock_send_udp;
    pthread_mutex_t lock_game_board;
} udp_thread_data;

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
static struct sockaddr_in6 *addr_mult = NULL;

static tcp_thread_data *tcp_threads_data_players[PLAYER_NUM];
static pthread_t game_threads[3];

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

uint16_t get_random_port() {
    return htons(MIN_PORT + (random() % (MAX_PORT - MIN_PORT)));
}

uint16_t get_port_tcp() {
    return ntohs(port_tcp);
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

int try_to_bind_random_port_on_socket_udp() {
    int res = try_to_bind_random_port_on_socket(sock_udp);
    RETURN_FAILURE_IF_ERROR(res);
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

void free_addr_mult() {
    if (addr_mult != NULL) {
        free(addr_mult);
        addr_mult = NULL;
    }
}

int init_addr_mult() {
    addr_mult = malloc(sizeof(struct sockaddr_in6));
    RETURN_FAILURE_IF_NULL_PERROR(addr_mult, "malloc addr_mult");

    memset(addr_mult, 0, sizeof(struct sockaddr_in6));
    addr_mult->sin6_family = AF_INET6;
    addr_mult->sin6_port = port_mult;

    char *addr_string = convert_adrmdif_into_string(adrmdiff);
    int res = inet_pton(AF_INET6, addr_string, &addr_mult->sin6_addr);
    free(addr_string);

    if (res < 0) {
        free_addr_mult();
        perror("inet_pton addr_mult");
        return EXIT_FAILURE;
    }

    int ifindex = if_nametoindex("eth0");
    if (ifindex < 0) {
        free_addr_mult();
        perror("if_nametoindex eth0");
        return EXIT_FAILURE;
    }

    addr_mult->sin6_scope_id = ifindex;
    return EXIT_SUCCESS;
}

int init_server_network(uint16_t connexion_port) {
    RETURN_FAILURE_IF_ERROR(init_socket_tcp());
    RETURN_FAILURE_IF_ERROR(init_socket_udp());
    RETURN_FAILURE_IF_ERROR(init_socket_mult());

    if (connexion_port >= MIN_PORT && connexion_port <= MAX_PORT) {
        if (try_to_bind_port_on_socket_tcp(connexion_port) != EXIT_SUCCESS) {
            fprintf(stderr, "The connexion port not works, try another one.\n");
            goto exit_closing_sockets;
        }
    } else if (try_to_bind_random_port_on_socket_tcp() != EXIT_SUCCESS) {
        goto exit_closing_sockets;
    }
    if (try_to_bind_random_port_on_socket_udp() != EXIT_SUCCESS) {
        goto exit_closing_sockets;
    }
    if (init_random_port_on_socket_mult() != EXIT_SUCCESS) {
        goto exit_closing_sockets;
    }
    if (init_random_adrmdiff() == EXIT_FAILURE) {
        goto exit_closing_sockets;
    }
    if (init_addr_mult() == EXIT_FAILURE) {
        goto exit_closing_sockets;
    }
    return EXIT_SUCCESS;

exit_closing_sockets:
    close_socket_tcp();
    close_socket_udp();
    close_socket_mult();
    return EXIT_FAILURE;
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
        if (data_thread[i] == NULL) {
            free(data_thread[i]);
            data_thread[i] = NULL;
        }
    }
    free(data_thread);
}

int init_tcp_threads_data() {
    // TODO CHANGE FOR MULTIGAME
    unsigned *ready_player_number = malloc(sizeof(unsigned));
    *ready_player_number = 0;
    unsigned *connected_players = malloc(sizeof(unsigned));
    *connected_players = 0;
    pthread_mutex_t *lock_waiting_all_players_join = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_t *lock_all_players_ready = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_t *lock_waiting_the_game_finish = malloc(sizeof(pthread_mutex_t));
    pthread_cond_t *cond_lock_waiting_all_players_join = malloc(sizeof(pthread_cond_t));
    pthread_cond_t *cond_lock_all_players_ready = malloc(sizeof(pthread_cond_t));
    pthread_cond_t *cond_lock_waiting_the_game_finish = malloc(sizeof(pthread_cond_t));
    if (pthread_mutex_init(lock_waiting_the_game_finish, NULL) < 0) {
        return EXIT_FAILURE;
    }
    if (pthread_mutex_init(lock_all_players_ready, NULL) < 0) {
        return EXIT_FAILURE;
    }
    if (pthread_mutex_init(lock_waiting_all_players_join, NULL) < 0) {
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
    for (unsigned i = 0; i < PLAYER_NUM; i++) {
        tcp_threads_data_players[i] = malloc(sizeof(tcp_thread_data));

        if (tcp_threads_data_players[i] == NULL) {
            perror("malloc tcp_thread_data");
            for (unsigned j = 0; j < i; j++) {
                free(tcp_threads_data_players[j]);
            }
            return EXIT_FAILURE;
        }
        memset(tcp_threads_data_players[i], 0, sizeof(tcp_thread_data));
        tcp_threads_data_players[i]->ready_player_number = ready_player_number;
        tcp_threads_data_players[i]->connected_players = connected_players;
        tcp_threads_data_players[i]->lock_waiting_all_players_join = lock_waiting_all_players_join;
        tcp_threads_data_players[i]->lock_all_players_ready = lock_all_players_ready;
        tcp_threads_data_players[i]->lock_waiting_the_game_finish = lock_waiting_the_game_finish;
        tcp_threads_data_players[i]->cond_lock_waiting_all_players_join = cond_lock_waiting_all_players_join;
        tcp_threads_data_players[i]->cond_lock_all_players_ready = cond_lock_all_players_ready;
        tcp_threads_data_players[i]->cond_lock_waiting_the_game_finish = cond_lock_waiting_the_game_finish;
    }
    return EXIT_SUCCESS;
}

void *serve_client_tcp(void *);

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


initial_connection_header *recv_initial_connection_header_of_client(int id) {
    return recv_initial_connection_header(sock_clients[id]);
}

ready_connection_header *recv_ready_connexion_header_of_client(int id) {
    return recv_ready_connexion_header(sock_clients[id]);
}

game_action *recv_game_action_of_clients() {
    return recv_game_action(sock_udp);
}

int send_connexion_information_of_client(int id, int eq) {
    // TODO Replace the gamemode
    return send_connexion_information(sock_clients[id], SOLO, id, eq, ntohs(port_udp), ntohs(port_mult), adrmdiff);
}

int send_game_board_for_clients(uint16_t num, board *board_) {
    return send_game_board(sock_mult, addr_mult, num, board_);
}

int send_game_update_for_clients(uint16_t num, tile_diff *diff, uint8_t nb) {
    return send_game_update(sock_mult, addr_mult, num, diff, nb);
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

int try_to_init_socket_of_client(int id) {
    if (id < 0 || id >= PLAYER_NUM) {
        return EXIT_FAILURE;
    }
    struct sockaddr_in6 client_addr;
    int client_addr_len = sizeof(client_addr);
    int res = accept(sock_tcp, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
    if (res < 0) {
        perror("client acceptance");
        return EXIT_FAILURE;
    }
    sock_clients[id] = res;

    print_ip_of_client(client_addr);
    return EXIT_SUCCESS;
}

int init_game_model(GAME_MODE mode, unsigned id) {
    dimension dim;
    dim.width = GAMEBOARD_WIDTH;
    dim.height = GAMEBOARD_HEIGHT;

    return init_model(dim, mode, id);
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
    while (!data->finished_flag) {
        game_action *action = recv_game_action_of_clients();
        if (action != NULL && action->game_mode == get_game_mode(data->game_id)) {
            pthread_mutex_lock(&data->lock_game_actions);
            add_game_action_to_thread_data(data, action);
            pthread_mutex_unlock(&data->lock_game_actions);
        }
    }
    return NULL;
}

void increment_last_num_message(int *last_num_message) {
    *last_num_message = *last_num_message + 1 % LIMIT_LAST_NUM_MESSAGE_MULT;
}

void *serve_clients_send_mult_sec(void *arg_udp_thread_data) {
    udp_thread_data *data = (udp_thread_data *)arg_udp_thread_data;

    int last_num_sec_message = 1;
    while (!data->finished_flag) {
        sleep(1);
        update_bombs(data->game_id);

        pthread_mutex_lock(&data->lock_game_board);
        board *game_board = get_game_board(data->game_id);
        pthread_mutex_unlock(&data->lock_game_board);
        RETURN_NULL_IF_NULL(game_board);

        pthread_mutex_lock(&data->lock_send_udp);
        send_game_board_for_clients(last_num_sec_message, game_board);
        pthread_mutex_unlock(&data->lock_send_udp);
        increment_last_num_message(&last_num_sec_message);
        free(game_board);
        // TODO manage errors
    }
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
    while (!data->finished_flag) {
        usleep(FREQ);

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
        pthread_mutex_lock(&data->lock_game_board);
        unsigned size_tile_diff = 0;
        tile_diff *diffs = update_game_board(data->game_id, player_actions, nb_player_actions, &size_tile_diff);
        RETURN_NULL_IF_NULL(diffs);
        pthread_mutex_unlock(&data->lock_game_board);
        if (size_tile_diff == 0) {
            continue;
        }
        // Send the differences
        pthread_mutex_lock(&data->lock_send_udp);
        send_game_update_for_clients(last_num_freq_message, diffs, size_tile_diff);
        pthread_mutex_unlock(&data->lock_send_udp);

        // Last free
        free(diffs);
        free_game_actions(game_actions, nb_game_actions);

        // Prepare new message
        increment_last_num_message(&last_num_freq_message);
    }
    return NULL;
}

int init_game_threads(unsigned id) {
    udp_thread_data *udp_thread_data_game = malloc(sizeof(udp_thread_data));
    RETURN_FAILURE_IF_NULL(udp_thread_data_game);
    udp_thread_data_game->finished_flag = false;
    udp_thread_data_game->game_id = id;
    udp_thread_data_game->game_actions = NULL;
    udp_thread_data_game->size_game_actions = 0;
    udp_thread_data_game->nb_game_actions = 0;

    if (pthread_mutex_init(&udp_thread_data_game->lock_game_actions, NULL) < 0) {
        goto EXIT_FREEING_DATA;
    }
    if (pthread_mutex_init(&udp_thread_data_game->lock_send_udp, NULL) < 0) {
        goto EXIT_FREEING_DATA;
    }
    if (pthread_mutex_init(&udp_thread_data_game->lock_game_board, NULL) < 0) {
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


void wait_all_clients_connected(pthread_mutex_t *lock, pthread_cond_t *cond, bool is_everyone_connected, unsigned *nb_connected) {
    if (!is_everyone_connected) {
        pthread_cond_wait(cond, lock);
    } else {
        free(nb_connected);
        pthread_cond_broadcast(cond);
    }
    pthread_mutex_unlock(lock);
}

void wait_all_clients_not_ready(pthread_mutex_t *lock, pthread_cond_t *cond, bool is_ready, unsigned *nb_ready) {
    if (!is_ready) {
        pthread_cond_wait(cond, lock);
    } else {
        board *game_board = get_game_board(TMP_GAME_ID);
        send_game_board_for_clients(0, game_board); // Initial game_board send
        free(game_board);
        init_game_threads(TMP_GAME_ID); // TODO MANAGE ERRORS
        free(nb_ready);
        pthread_cond_broadcast(cond);
    }
    pthread_mutex_unlock(lock);
}

void *serve_client_tcp(void *arg_tcp_thread_data) {
    tcp_thread_data *tcp_data = (tcp_thread_data *)arg_tcp_thread_data;
    
    pthread_mutex_lock(tcp_data->lock_waiting_all_players_join);
    *(tcp_data->connected_players) += 1;
    printf("%d\n", *tcp_data->connected_players);
    bool is_everyone_connected = *tcp_data->connected_players == PLAYER_NUM;
    wait_all_clients_connected(tcp_data->lock_waiting_all_players_join, tcp_data->cond_lock_waiting_all_players_join, is_everyone_connected, tcp_data->connected_players);
    printf("%d send\n", tcp_data->id);
    send_connexion_information_of_client(tcp_data->id, 0);
    printf("%d after send\n", tcp_data->id);

    // TODO verify ready_informations
    ready_connection_header *ready_informations = recv_ready_connexion_header_of_client(tcp_data->id);

    print_ready_player(ready_informations);
    pthread_mutex_lock(tcp_data->lock_all_players_ready);
    *(tcp_data->ready_player_number) += 1;
    
    bool is_ready = *tcp_data->ready_player_number == PLAYER_NUM;
    wait_all_clients_not_ready(tcp_data->lock_all_players_ready, tcp_data->cond_lock_all_players_ready, is_ready,
                               tcp_data->ready_player_number);

    // TODO end of the game
    lock_mutex_to_wait(tcp_data->lock_waiting_the_game_finish, tcp_data->cond_lock_waiting_the_game_finish);

    printf("Player %d left the game.\n", ready_informations->id);
    free(ready_informations);
    close_socket_client(tcp_data->id);
    // TODO keep free(tcp_data) if we don't use join
    // TO CONTINUE
    return NULL;
}

int connect_one_player_to_game(int sock, unsigned *connected_player_solo, unsigned *connected_player_eq) {
    // TODO change recv not tcp_data
    sock_clients[*connected_player_solo] = sock;
    initial_connection_header *head = recv_initial_connection_header_of_client(*connected_player_solo);

    if (head->game_mode == SOLO) {
        // TODO INIT SOLO GAME
        if (*connected_player_solo == 0) {
            RETURN_FAILURE_IF_ERROR(init_game_model(SOLO, TMP_GAME_ID)); // TODO Change it to run more than 1 server
        }
        tcp_threads_data_players[*connected_player_solo]->id = *connected_player_solo;
        *connected_player_solo += 1;
    } else if (head->game_mode == TEAM && *connected_player_eq == 0) {
        // TODO INIT TEAM GAME
        if (*connected_player_eq == 0) {
            RETURN_FAILURE_IF_ERROR(init_game_model(TEAM, TMP_GAME_ID)); // TODO Change it to run more than 1 server
        }
        tcp_threads_data_players[*connected_player_eq]->id = *connected_player_eq;
        *connected_player_eq += 1;
    }
    free(head);

    // DO CASE EQ
    pthread_t t;
    if (pthread_create(&t, NULL, serve_client_tcp, tcp_threads_data_players[*connected_player_solo - 1]) < 0) {
        perror("thread creation");
        return EXIT_FAILURE;
    }

    if (*connected_player_solo == PLAYER_NUM) {
        // New game
    }
    return EXIT_SUCCESS;
}

int connect_player_to_game() {
    listen_players();
    printf("Waiting players on %u port.\n", get_port_tcp());

    struct pollfd *polls = init_polls_connexion(sock_tcp);
    unsigned nbfds = 1;
    unsigned s = INITIAL_POLL_FD_SIZE;

    unsigned connected_player_solo = 0;
    unsigned connected_player_eq = 0;

    int connected = 0; // TODO CHANGE

    // TODO Add 2 tcp thread data
    init_tcp_threads_data();

    while (1) {
        poll(polls, nbfds, -1);

        for (unsigned i = 1; i < nbfds; i++) {
            if (polls[i].revents & POLL_IN) {
                connect_one_player_to_game(polls[i].fd, &connected_player_solo, &connected_player_eq);
                remove_polls_to_poll(polls, &nbfds, i);
            }
            if (polls[i].revents & POLL_HUP) {
                close(polls[i].fd);
                remove_polls_to_poll(polls, &nbfds, i);
            }
        }
        if (polls[0].revents & POLL_IN) {
            try_to_init_socket_of_client(connected);
            // TODO Change sock
            add_polls_to_poll(polls, &nbfds, &s, sock_clients[connected]);
            connected++;
        }

        // TODO verify well send
    }

    printf("All players are connected.\n");
    return EXIT_SUCCESS;
}

int game_loop_server() {
    int return_value;
    if (connect_player_to_game() != EXIT_SUCCESS) {
        return_value = EXIT_FAILURE;
        goto exit_closing_sockets_and_free_addr_mult;
    }
    sleep(1); // Wait all clients for the join mutex
    goto exit_closing_sockets_and_free_addr_mult;

exit_closing_sockets_and_free_addr_mult:
    close_socket_tcp();
    close_socket_udp();
    close_socket_mult();
    free_addr_mult();
    return return_value;
}
