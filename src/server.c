#include <pthread.h>
#include <stdbool.h>
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
    pthread_mutex_t lock_game_board;
} udp_thread_data;

#define TMP_GAME_ID 0
#define FREQ 50000 // 100 00 us = 10 ms
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

#define LIMIT_LAST_NUM_MESSAGE_MULT ((1 << 15) - 1)   // 2^16
#define LIMIT_LAST_NUM_MESSAGE_CLIENT ((1 << 12) - 1) // 2^13

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
    lock_mutex_to_wait(&lock_waiting_all_players_join, &cond_lock_waiting_all_players_join);
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
    lock_mutex_to_wait(&lock_waiting_the_game_finish, &cond_lock_waiting_the_game_finish);

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
            printf("action: %d %d %d %d %d\n", action->action, action->eq, action->id, action->game_mode,
                   action->message_number);
            pthread_mutex_lock(&data->lock_game_actions);
            add_game_action_to_thread_data(data, action);
            printf("nb game actions %d\n", data->nb_game_actions);
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
        printf("i : %d\n", i);
        // Other actions were sent before those kept
        if (nb_player_moves == PLAYER_NUM && nb_place_bomb == PLAYER_NUM) {
            break;
        }
        // Message ignored
        if (!is_next_message(last_num_received_message[game_actions[i]->id], game_actions[i]->message_number,
                             LIMIT_LAST_NUM_MESSAGE_CLIENT)) {
            printf("is_not_next_message\n");
            continue;
        }
        // Keep the action if its a move and there is no move kept for this player
        if (is_move(game_actions[i]->action) && !already_move[game_actions[i]->id]) {
            printf("move\n");
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
    for (unsigned i = 0; i < nb_player_moves; i++) {
        printf("player_moves : %d, %d\n", player_moves[i].id, player_moves[i].action);
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
        printf("-----> Sending freq message %d\n", last_num_freq_message);

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
        printf("nb_diffs : %d\n", size_tile_diff);
        for (unsigned i = 0; i < size_tile_diff; i++) {
            printf("diff : %d, %d, %d\n", diffs[i].tile, diffs[i].x, diffs[i].y);
        }
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

int main() {
    srandom(time(NULL));
    int return_value = EXIT_SUCCESS;
    RETURN_FAILURE_IF_ERROR(init_server_network());
    if (connect_player_to_game() != EXIT_SUCCESS) {
        return_value = EXIT_FAILURE;
        goto exit_closing_sockets_and_free_addr_mult;
    }
    sleep(1); // Wait all clients for the join mutex
    unlock_mutex_for_everyone(&lock_waiting_all_players_join, &cond_lock_waiting_all_players_join);
    wait_all_clients_not_ready();
    RETURN_FAILURE_IF_ERROR(init_game_model(SOLO, TMP_GAME_ID)); // TODO Change it to run more than 1 server

    board *game_board = get_game_board(TMP_GAME_ID);

    send_game_board_for_clients(0, game_board); // Initial game_board send
    free(game_board);
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
