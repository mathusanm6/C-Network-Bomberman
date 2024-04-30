#include "./controller.h"

#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>

#include "./model.h"
#include "./utils.h"
#include "./view.h"

#define KEY_BACKSPACE_1 0407
#define KEY_BACKSPACE_2 127
#define KEY_ENTER_GENERAL 10
#define KEY_ENTER_NUMERIC 0527
#define KEY_BACKSLASH '\\'
#define KEY_SPACE ' '
#define KEY_TILDA '~'
#define KEY_VERTICAL_BAR '|'
#define KEY_CONTROL_D 4
#define KEY_TAB_1 KEY_STAB
#define KEY_TAB_2 '\t'

/* TODO: fixme once multiple games are supported */
#define TMP_GAME_ID 0

static int current_player = 0;

static void switch_player() {
    clear_line(TMP_GAME_ID);
    do {
        current_player = (current_player + 1) % PLAYER_NUM;
    } while (is_player_dead(current_player, TMP_GAME_ID));
}

void init_controller() {
    intrflush(stdscr, FALSE); /* No need to flush when intr key is pressed */
    keypad(stdscr, TRUE);     /* Required in order to get events from keyboard */
    nodelay(stdscr, TRUE);    /* Make getch non-blocking */
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
            a = GAME_ACTIVATE_CHAT;
            break;
        case KEY_TILDA:
            a = GAME_QUIT;
            break;
        case KEY_VERTICAL_BAR:
            a = GAME_SWITCH_PLAYER;
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
            a = CHAT_QUIT;
            break;
        case KEY_TILDA:
            a = CHAT_GAME_QUIT;
            break;
        case KEY_VERTICAL_BAR:
            a = CHAT_SWITCH_PLAYER;
            break;
        default:
            a = CHAT_WRITE;
            break;
    }

    return a;
}

int get_pressed_key() {
    int c;
    int prev_c = ERR;
    // We consume all similar consecutive key presses
    while ((c = getch()) != ERR) { // getch returns the first key press in the queue
        if (prev_c != ERR && prev_c != c) {
            ungetch(c); // put 'c' back in the queue
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
            add_to_line(c, TMP_GAME_ID);
            break;
        case CHAT_ERASE:
            decrement_line(TMP_GAME_ID);
            break;
        case CHAT_SEND:
            if (add_message(current_player, TMP_GAME_ID) == EXIT_SUCCESS) {
                clear_line(TMP_GAME_ID);
            } else {
                perror("Error sending message");
            }
            break;
        case CHAT_TOGGLE_WHISPER:
            toggle_whispering(TMP_GAME_ID);
            break;
        case CHAT_CLEAR:
            clear_line(TMP_GAME_ID);
            break;
        case CHAT_QUIT:
            set_chat_focus(false, TMP_GAME_ID);
            break;
        case CHAT_GAME_QUIT:
            return true;
        case CHAT_SWITCH_PLAYER:
            switch_player();
            break;
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
            perform_move(a, current_player, TMP_GAME_ID);
            break;
        case GAME_PLACE_BOMB:
            place_bomb(current_player, TMP_GAME_ID);
            break;
        case GAME_ACTIVATE_CHAT:
            set_chat_focus(true, TMP_GAME_ID);
            break;
        case GAME_QUIT:
            return true;
        case GAME_SWITCH_PLAYER:
            switch_player();
            break;
        case GAME_NONE:
            break;
    }

    return false;
}

bool control() {
    int c = get_pressed_key();

    if (is_chat_on_focus(TMP_GAME_ID)) {
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

int init_game() {
    RETURN_FAILURE_IF_ERROR(init_view());

    init_controller();

    dimension dim;
    get_computed_board_dimension(&dim);

    return init_model(dim, SOLO, TMP_GAME_ID);
}

int game_loop() {
    while (true) {
        if (is_game_over(TMP_GAME_ID) || control()) {
            break;
        }
        board *game_board = get_game_board(TMP_GAME_ID);
        chat *chat_ = get_chat(TMP_GAME_ID);
        refresh_game(game_board, chat_, current_player);
        free_board(game_board);
        update_bombs(TMP_GAME_ID);
        usleep(30 * 1000);
    }
    free_model(TMP_GAME_ID);
    end_view();

    return EXIT_SUCCESS;
}