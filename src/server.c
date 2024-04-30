#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "messages.h"
#include "model.h"
#include "network_server.h"
#include "utils.h"

typedef struct tcp_thread_data {
    unsigned id;
    pthread_t thread_id;
    bool finished_flag;
} tcp_thread_data;

typedef struct udp_thread_data {
    unsigned game_id;
    game_action **game_actions;
    unsigned nb_game_actions;
    unsigned size_game_actions;
    bool finished_flag;
    pthread_mutex_t lock_game_actions;
    pthread_mutex_t lock_send_udp;
} udp_thread_data;

#define TMP_GAME_ID 0
#define FREQ 10000 // 100 00 us = 10 ms
#define INITIAL_GAME_ACTIONS_SIZE 4

static tcp_thread_data *tcp_threads_data_players[PLAYER_NUM];
static pthread_t game_threads[3];

pthread_mutex_t lock_waiting_all_players_join = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_all_players_ready = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_waiting_the_game_finish = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t cond_lock_waiting_all_players_join = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_lock_all_players_ready = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_lock_waiting_the_game_finish = PTHREAD_COND_INITIALIZER;

static unsigned connected_player_number = 0;
static unsigned ready_player_number = 0;

#define LIMIT_LAST_NUM_MESSAGE_MULT 65536 // 2^16

void *serve_client_tcp(void *);

void free_tcp_threads_data() {
    for (unsigned i = 0; i < connected_player_number; i++) {
        if (tcp_threads_data_players[i] != NULL) {
            free(tcp_threads_data_players[i]);
            tcp_threads_data_players[i] = NULL;
        }
    }
}

int init_server_network() {
    RETURN_FAILURE_IF_ERROR(init_socket_tcp());
    RETURN_FAILURE_IF_ERROR(init_socket_udp());
    RETURN_FAILURE_IF_ERROR(init_socket_mult());

    if (try_to_bind_random_port_on_socket_tcp() != EXIT_SUCCESS) {
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

int init_tcp_threads_data() {
    for (unsigned i = 0; i < PLAYER_NUM; i++) {
        tcp_threads_data_players[i] = malloc(sizeof(tcp_thread_data));

        if (tcp_threads_data_players[i] == NULL) {
            perror("malloc tcp_thread_data");
            for (unsigned j = 0; j < i; j++) {
                free(tcp_threads_data_players[j]);
            }
            return EXIT_FAILURE;
        }
        memset(&tcp_threads_data_players[i], 0, sizeof(tcp_thread_data));
    }
    return EXIT_SUCCESS;
}

void lock_mutex(pthread_mutex_t *mutex, pthread_cond_t *cond) {
    pthread_mutex_lock(mutex);
    pthread_cond_wait(cond, mutex);
    pthread_mutex_unlock(mutex);
}

void unlock_mutex_for_everyone(pthread_mutex_t *mutex, pthread_cond_t *cond) {
    pthread_mutex_lock(mutex);
    pthread_cond_broadcast(cond);
    pthread_mutex_unlock(mutex);
}

void wait_all_clients_not_ready() {
    if (ready_player_number < PLAYER_NUM) {
        pthread_cond_wait(&cond_lock_all_players_ready, &lock_all_players_ready);
    } else {
        pthread_cond_broadcast(&cond_lock_all_players_ready);
    }
    pthread_mutex_unlock(&lock_all_players_ready);
}

void init_connexion_with_client(tcp_thread_data *tcp_data) {
    // TODO separate solo and eq client
    initial_connection_header *head = recv_initial_connection_header_of_client(tcp_data->id);
    free(head);
    lock_mutex(&lock_waiting_all_players_join, &cond_lock_waiting_all_players_join);
    // TODO verify well send
    send_connexion_information_of_client(tcp_data->id, 0);
}

void print_ready_player(ready_connection_header *ready_informations) {
    printf("Player with Id : %d, eq : %d is ready.\n", ready_informations->id, ready_informations->eq);
}

void *serve_client_tcp(void *arg_tcp_thread_data) {
    tcp_thread_data *tcp_data = (tcp_thread_data *)arg_tcp_thread_data;

    init_connexion_with_client(tcp_data);

    // TODO verify ready_informations
    ready_connection_header *ready_informations = recv_ready_connexion_header_of_client(tcp_data->id);
    print_ready_player(ready_informations);
    pthread_mutex_lock(&lock_all_players_ready);
    ready_player_number++;

    wait_all_clients_not_ready();

    // TODO end of the game
    lock_mutex(&lock_waiting_the_game_finish, &cond_lock_waiting_the_game_finish);

    printf("Player %d left the game.\n", ready_informations->id);
    free(ready_informations);
    close_socket_client(tcp_data->id);
    // TODO keep free(tcp_data) if we don't use join
    // TO CONTINUE
    return NULL;
}

int connect_one_player_to_game(int id) {
    tcp_threads_data_players[id] = malloc(sizeof(tcp_threads_data_players));
    tcp_threads_data_players[id]->id = id;

    int res = try_to_init_socket_of_client(id);

    if (res < 0) {
        return EXIT_FAILURE;
    }

    if (pthread_create(&tcp_threads_data_players[id]->thread_id, NULL, serve_client_tcp, tcp_threads_data_players[id]) <
        0) {
        perror("thread creation");
        return EXIT_FAILURE;
    }
    connected_player_number++;
    return EXIT_SUCCESS;
}

int connect_player_to_game() {
    listen_players();
    printf("Waiting players on %u port.\n", get_port_tcp());
    while (connected_player_number < PLAYER_NUM) {
        connect_one_player_to_game(connected_player_number);
    }
    printf("All players are connected.\n");
    return EXIT_SUCCESS;
}

void join_tcp_threads() {
    for (unsigned i = 0; i < PLAYER_NUM; i++) {
        pthread_join(tcp_threads_data_players[i]->thread_id, NULL);
    }
    free_tcp_threads_data();
}

void increment_last_num_message(int *last_num_message) {
    *last_num_message = *last_num_message + 1 % LIMIT_LAST_NUM_MESSAGE_MULT;
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

void *serv_client_recv_game_action(void *arg_udp_thread_data) {
    udp_thread_data *data = (udp_thread_data *)arg_udp_thread_data;
    while (!data->finished_flag) {
        game_action *action = recv_game_action_of_clients();
        if (action != NULL) {
            pthread_mutex_lock(&data->lock_game_actions);
            add_game_action_to_thread_data(data, action);
            pthread_mutex_unlock(&data->lock_game_actions);
        }
    }
    return NULL;
}

void *serve_clients_send_mult_sec(void *arg_udp_thread_data) {
    udp_thread_data *data = (udp_thread_data *)arg_udp_thread_data;

    int last_num_sec_message = 1;
    while (!data->finished_flag) {
        sleep(1);
        pthread_mutex_lock(&data->lock_send_udp);
        send_game_board_for_clients(last_num_sec_message, get_game_board(data->game_id));
        pthread_mutex_unlock(&data->lock_send_udp);
        increment_last_num_message(&last_num_sec_message);
        // TODO manage errors
    }
    return NULL;
}

int manage_game_actions(udp_thread_data *data, int id, int *last_num_received_message) {
    GAME_ACTION last_move = GAME_NONE;
    int last_move_num = *last_num_received_message;
    int last_action_num = *last_num_received_message;
    bool to_place_bomb = false;

    for (unsigned i = 0; i < data->nb_game_actions; i++) {
        if (data->game_actions[i]->id == id) {
            GAME_ACTION action = data->game_actions[i]->action;
            if (action == GAME_PLACE_BOMB) {
                to_place_bomb = true;
            }
            if ((action == GAME_UP || action == GAME_RIGHT || action == GAME_LEFT || action == GAME_DOWN) &&
                abs(*last_num_received_message - last_move_num) <
                    abs(*last_num_received_message - data->game_actions[i]->message_number)) {
                last_move = action;
                last_move_num = data->game_actions[i]->message_number;
            }
            if (abs(*last_num_received_message - last_action_num) <
                abs(*last_num_received_message - data->game_actions[i]->message_number)) {
                last_action_num = data->game_actions[i]->message_number;
            }
        }
    }
    if (to_place_bomb) {
        place_bomb(id, data->game_id);
    }
    if (last_move != GAME_NONE) {
        perform_move(last_move, id, data->game_id);
    }
    return EXIT_SUCCESS;
}

void *serve_clients_send_mult_freq(void *arg_udp_thread_data) {
    udp_thread_data *data = (udp_thread_data *)arg_udp_thread_data;
    int last_num_received_messages[PLAYER_NUM];

    int last_num_freq_message = 0;
    while (!data->finished_flag) {
        usleep(FREQ);
        board *current_board = get_game_board(data->game_id);

        pthread_mutex_lock(&data->lock_game_actions);
        for (unsigned id = 1; id <= PLAYER_NUM; id++) {
            manage_game_actions(data, id, &last_num_received_messages[id - 1]);
        }
        empty_game_actions(data);
        pthread_mutex_unlock(&data->lock_game_actions);
        update_bombs(data->game_id);

        unsigned size = 0;
        tile_diff *diffs = get_diff_with_board(data->game_id, current_board, &size);
        if (diffs != NULL) {
            pthread_mutex_lock(&data->lock_send_udp);
            send_game_update_for_clients(last_num_freq_message, diffs, size);
            pthread_mutex_unlock(&data->lock_send_udp);
        }
        free(diffs);
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

int main() {
    srandom(time(NULL));
    int return_value = EXIT_SUCCESS;
    RETURN_FAILURE_IF_ERROR(init_server_network());
    if (connect_player_to_game() != EXIT_SUCCESS) {
        return_value = EXIT_FAILURE;
        goto exit_closing_sockets_and_free_addr_mult;
    }
    sleep(1);                                                    // Wait all clients for the join mutex
    RETURN_FAILURE_IF_ERROR(init_game_model(SOLO, TMP_GAME_ID)); // TODO Change it to run more than 1 server
    unlock_mutex_for_everyone(&lock_waiting_all_players_join, &cond_lock_waiting_all_players_join);

    send_game_board_for_clients(0, get_game_board(TMP_GAME_ID)); // Initial game_board send
    if (init_game_threads(TMP_GAME_ID) != EXIT_SUCCESS) {
        return_value = EXIT_FAILURE;
        goto exit_closing_sockets_and_free_addr_mult;
    }
    join_tcp_threads();
    goto exit_closing_sockets_and_free_addr_mult;

exit_closing_sockets_and_free_addr_mult:
    close_socket_tcp();
    close_socket_udp();
    close_socket_mult();
    free_addr_mult();
    return return_value;
}
