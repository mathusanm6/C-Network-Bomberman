#include "./controller.h"
#include "./communication_client.h"
#include "./messages.h"
#include "./network_client.h"
#include "./utils.h"
#include "./view.h"
#include "chat_model.h"
#include "model.h"

#include <ncurses.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define KEY_BACKSPACE_1 0407
#define KEY_BACKSPACE_2 127
#define KEY_ENTER_GENERAL 10
#define KEY_ENTER_NUMERIC 0527
#define KEY_BACKSLASH '\\'
#define KEY_SPACE ' '
#define KEY_TILDA '~'
#define KEY_CONTROL_D 4
#define KEY_TAB_1 KEY_STAB
#define KEY_TAB_2 '\t'

/* TODO: fixme once multiple games are supported */
#define TMP_GAME_ID 0

// TODO: Fix memory leak
static board *game_board = NULL;
static pthread_mutex_t game_board_mutex = PTHREAD_MUTEX_INITIALIZER;

static chat *client_chat = NULL;
static pthread_mutex_t chat_mutex = PTHREAD_MUTEX_INITIALIZER;

// TODO
static GAME_MODE game_mode = SOLO;
static int player_id = 0;

static int message_number = 0;
static int eq = 0;

static pthread_mutex_t view_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_controller() {
    intrflush(stdscr, FALSE); /* No need to flush when intr key is pressed */
    keypad(stdscr, TRUE);     /* Required in order to get events from keyboard */
    nodelay(stdscr, TRUE);    /* Make getch non-blocking */
    game_board = malloc(sizeof(board));
    RETURN_IF_NULL_PERROR(game_board, "malloc board_controller");
    // TODO: wait for first message and init with the correct dimensions
    game_board->dim = (dimension){49, 23};
    game_board->grid = malloc(game_board->dim.height * game_board->dim.width * sizeof(char));
    RETURN_IF_NULL_PERROR(game_board->grid, "malloc board_controller grid");
    for (int i = 0; i < game_board->dim.height * game_board->dim.width; i++) {
        game_board->grid[i] = EMPTY;
    }
    client_chat = create_chat();
    RETURN_IF_NULL_PERROR(client_chat, "create_chat");
}

GAME_ACTION key_press_to_game_action(int c) {
    GAME_ACTION a = GAME_NONE;
    switch (c) {
        case ERR:
            break;
        case KEY_UP:
            a = GAME_UP;
            break;
        case KEY_RIGHT:
            a = GAME_RIGHT;
            break;
        case KEY_DOWN:
            a = GAME_DOWN;
            break;
        case KEY_LEFT:
            a = GAME_LEFT;
            break;
        case KEY_SPACE:
            a = GAME_PLACE_BOMB;
            break;
        case KEY_BACKSLASH:
            a = GAME_CHAT_MODE_START;
            break;
        case KEY_TILDA:
            a = GAME_QUIT;
            break;
    }

    return a;
}

CHAT_ACTION key_press_to_chat_action(int c) {
    CHAT_ACTION a = CHAT_NONE;
    switch (c) {
        case ERR:
            break;
        case KEY_BACKSPACE_1:
        case KEY_BACKSPACE_2:
            a = CHAT_ERASE;
            break;
        case KEY_ENTER_GENERAL:
        case KEY_ENTER_NUMERIC:
            a = CHAT_SEND;
            break;
        case KEY_TAB_1:
        case KEY_TAB_2:
            a = CHAT_TOGGLE_WHISPER;
            break;
        case KEY_CONTROL_D:
            a = CHAT_CLEAR;
            break;
        case KEY_BACKSLASH:
            a = CHAT_MODE_QUIT;
            break;
        case KEY_TILDA:
            a = CHAT_GAME_QUIT;
            break;
        default:
            a = CHAT_WRITE;
            break;
    }

    return a;
}

int mutex_getch() {
    int c;
    pthread_mutex_lock(&view_mutex);
    c = getch();
    pthread_mutex_unlock(&view_mutex);
    return c;
}

int get_pressed_key() {
    int c;
    int prev_c = ERR;
    while ((c = mutex_getch()) != ERR) { // getch returns the first key press in the queue
        if (prev_c != ERR && prev_c != c) {
            pthread_mutex_lock(&view_mutex);
            ungetch(c); // put 'c' back in the queue
            pthread_mutex_unlock(&view_mutex);
            break;
        }
        prev_c = c;
    }
    return prev_c;
}

bool perform_chat_action(int c) {
    CHAT_ACTION a = key_press_to_chat_action(c);
    switch (a) {
        case CHAT_WRITE:
            add_to_line(client_chat, c);
            break;
        case CHAT_ERASE:
            decrement_line(client_chat);
            break;
        case CHAT_SEND:
            char *message = NULL;
            bool whispered = false;
            if (add_message_from_client(client_chat, player_id, &message, &whispered) == EXIT_SUCCESS) {
                // Send message to server
                if (whispered) {
                    send_chat_message_to_server(TEAM_M, strlen(message), message);
                } else {
                    send_chat_message_to_server(GLOBAL_M, strlen(message), message);
                }
                clear_line(client_chat);
            }
            break;
        case CHAT_TOGGLE_WHISPER:
            toggle_whispering(client_chat);
            break;
        case CHAT_CLEAR:
            clear_line(client_chat);
            break;
        case CHAT_MODE_QUIT:
            set_chat_focus(client_chat, false);
            break;
        case CHAT_GAME_QUIT:
            // TODO: this is for testing
            exit(1);
            return true;
        case CHAT_NONE:
            break;
    }

    return false;
}

bool perform_game_action(int c) {
    GAME_ACTION a = key_press_to_game_action(c);
    switch (a) {
        case GAME_UP:
        case GAME_RIGHT:
        case GAME_DOWN:
        case GAME_LEFT:
        case GAME_PLACE_BOMB:
            game_action *action = malloc(sizeof(game_action));
            if (action == NULL) {
                return false;
            }

            action->game_mode = game_mode;
            action->id = player_id;
            action->eq = eq;
            action->message_number = message_number;
            message_number = (message_number + 1) % (1 << 13);
            action->action = a;

            send_game_action(action);

            break;
        case GAME_CHAT_MODE_START:
            set_chat_focus(client_chat, true);
            break;
        case GAME_QUIT:
            // TODO: Quit connection
            return true;
        case GAME_NONE:
            break;
    }

    return false;
}

bool control() {
    int c = get_pressed_key();

    if (is_chat_on_focus(client_chat)) {
        if (perform_chat_action(c)) {
            return true;
        }
    } else {
        if (perform_game_action(c)) {
            return true;
        }
    }

    return false;
}

int init_game(int player_nb, int eq_, GAME_MODE mode) {
    RETURN_FAILURE_IF_ERROR(init_view());

    // TODO: init the chat

    init_controller();

    player_id = player_nb;
    eq = eq_;
    game_mode = mode;

    return EXIT_SUCCESS;
}

board *get_board() {
    board *b = malloc(sizeof(board));
    RETURN_NULL_IF_NULL_PERROR(b, "malloc board");

    pthread_mutex_lock(&game_board_mutex);
    b->dim = game_board->dim;
    b->grid = malloc(b->dim.height * b->dim.width);
    if (b->grid == NULL) {
        pthread_mutex_unlock(&game_board_mutex);
        free(b);
        return NULL;
    }
    for (int i = 0; i < b->dim.height * b->dim.width; i++) {
        b->grid[i] = game_board->grid[i];
    }
    pthread_mutex_unlock(&game_board_mutex);

    return b;
}

void update_board(board *b, TILE *grid) {
    for (int i = 0; i < b->dim.height * b->dim.width; i++) {
        b->grid[i] = grid[i];
    }
}

void update_tile_diff(board *b, tile_diff *diff, int size) {
    for (int i = 0; i < size; i++) {
        int pos = diff[i].y * b->dim.width + diff[i].x;
        b->grid[pos] = diff[i].tile;
    }
}

/** Updates the game board based on the server MESSAGE*/
void *game_board_info_thread_function() {
    // TODO : game end
    while (true) {
        received_game_message *received_message = recv_game_message();
        if (received_message == NULL || received_message->message == NULL) {
            // TODO: Handle error
            continue;
        }
        switch (received_message->type) {
            case GAME_BOARD_INFORMATION:
                game_board_information *info = deserialize_game_board(received_message->message);
                pthread_mutex_lock(&game_board_mutex);
                update_board(game_board, info->board);
                pthread_mutex_unlock(&game_board_mutex);
                free_game_board_information(info);
                break;
            case GAME_BOARD_UPDATE:
                game_board_update *update = deserialize_game_board_update(received_message->message);
                pthread_mutex_lock(&game_board_mutex);
                update_tile_diff(game_board, update->diff, update->nb);
                pthread_mutex_unlock(&game_board_mutex);
                free_game_board_update(update);
                break;
            default:
                printf("Unknown message type\n");
                /* TODO: Handle error */
                break;
        }

        board *b = get_board();
        pthread_mutex_lock(&view_mutex);
        refresh_game(b, client_chat, player_id);
        pthread_mutex_unlock(&view_mutex);
    }

    return NULL;
}

void *chat_message_thread_function() {
    while (true) {
        chat_message *chat_msg = recv_chat_message_from_server();
        if (chat_msg != NULL) {
            pthread_mutex_lock(&chat_mutex);
            if (chat_msg->type == GLOBAL_M) {
                add_message_from_server(client_chat, chat_msg->id, chat_msg->message, false);
            } else if (chat_msg->type == TEAM_M) {
                add_message_from_server(client_chat, chat_msg->id, chat_msg->message, true);
            }
            pthread_mutex_unlock(&chat_mutex);
            free(chat_msg->message);
            free(chat_msg);
        }

        board *b = get_board();
        pthread_mutex_lock(&view_mutex);
        refresh_game(b, client_chat, player_id);
        pthread_mutex_unlock(&view_mutex);
    }

    return NULL;
}

/** Sends to the server the performed action*/
void *view_thread_function() {
    while (true) {
        // TODO: Handle game quit
        if (control()) {
            break;
        }
    }

    return NULL;
}

int game_loop() {
    RETURN_FAILURE_IF_NULL(game_board);

    /* TODO: Handle possible errors here */
    pthread_t game_board_info_thread;
    pthread_create(&game_board_info_thread, NULL, game_board_info_thread_function, NULL);

    pthread_t chat_message_thread;
    pthread_create(&chat_message_thread, NULL, chat_message_thread_function, NULL);

    pthread_t view_thread;
    pthread_create(&view_thread, NULL, view_thread_function, NULL);

    pthread_join(game_board_info_thread, NULL);
    pthread_join(chat_message_thread, NULL);

    free_board(game_board);
    end_view();

    return EXIT_SUCCESS;
}
