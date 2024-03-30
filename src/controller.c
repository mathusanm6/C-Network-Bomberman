#include "./controller.h"
#include "./model.h"
#include "./view.h"

#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>

int current_player = 0;
bool is_chat_on_focus = false;

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
        case ' ':
            a = GAME_PLACE_BOMB;
            break;
        case '\\':
            a = GAME_ACTIVATE_CHAT;
            break;
        case '~':
            a = GAME_QUIT;
            break;
        case '|':
            a = SWITCH_PLAYER;
            break;
    }

    return a;
}

CHAT_ACTION key_press_to_chat_action(int c) {
    CHAT_ACTION a = CHAT_NONE;
    switch (c) {
        case ERR:
            break;
        case KEY_BACKSPACE:
        case 127:
            a = CHAT_ERASE;
            break;
        case KEY_ENTER:
        case '\n':
            a = CHAT_SEND;
            break;
        case '-':
            a = CHAT_CLEAR;
            break;
        case '\\':
            a = CHAT_QUIT;
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

bool control() {
    int c = get_pressed_key();

    if (c == KEY_RESIZE) {
        // If the terminal was resized, quit the application
        endwin(); // Clean up ncurses
        printf("Terminal resized. Quitting application.\n");
        exit(EXIT_SUCCESS);
    }

    if (is_chat_on_focus) {
        CHAT_ACTION a = key_press_to_chat_action(c);
        switch (a) {
            case CHAT_WRITE:
                add_to_line(c);
                break;
            case CHAT_ERASE:
                decrement_line();
                break;
            case CHAT_SEND:
                // TODO : ADD MESSAGE TO CHAT
                break;
            case CHAT_CLEAR:
                clear_line();
                break;
            case CHAT_QUIT:
                is_chat_on_focus = false;
            case CHAT_NONE:
                break;
        }
        return false;
    } else {
        GAME_ACTION a = key_press_to_game_action(c);
        switch (a) {
            case GAME_UP:
            case GAME_RIGHT:
            case GAME_DOWN:
            case GAME_LEFT:
                perform_move(a, current_player);
                break;
            case GAME_PLACE_BOMB:
                // TODO
                break;
            case GAME_ACTIVATE_CHAT:
                is_chat_on_focus = true;
                break;
            case GAME_QUIT:
                return true;
            case SWITCH_PLAYER:
                current_player = (current_player + 1) % PLAYER_NUM;
                break;
            case GAME_NONE:
                break;
        }
    }

    return false;
}

int init_game() {
    init_view();
    init_controller();

    dimension dim;
    get_height_width_playable(&dim);
    padding pad = {PADDING_PLAYABLE_TOP, PADDING_PLAYABLE_TOP, PADDING_PLAYABLE_LEFT, PADDING_PLAYABLE_LEFT};
    add_padding(&dim, pad);
    if (init_model(dim, SOLO) < 0) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int game_loop() {
    while (true) {
        if (control()) {
            break;
        }
        board *game_board = get_game_board();
        refresh_game(game_board, chat_line);
        free_board(game_board);
        usleep(30 * 1000);
    }
    free_model();
    end_view();

    return EXIT_SUCCESS;
}
