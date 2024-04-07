#include "./controller.h"
#include "./model.h"
#include "./view.h"

#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>

int current_player = 0;

void init_controller() {
    intrflush(stdscr, FALSE); /* No need to flush when intr key is pressed */
    keypad(stdscr, TRUE);     /* Required in order to get events from keyboard */
    nodelay(stdscr, TRUE);    /* Make getch non-blocking */
}

ACTION key_press_to_action(int c) {
    ACTION a = NONE;
    switch (c) {
        case ERR:
            break;
        case KEY_UP:
            a = UP;
            break;
        case KEY_RIGHT:
            a = RIGHT;
            break;
        case KEY_DOWN:
            a = DOWN;
            break;
        case KEY_LEFT:
            a = LEFT;
            break;
        case ' ':
            a = PLACE_BOMB;
            break;
        case KEY_BACKSPACE:
            a = CHAT_ERASE;
            break;
        case '~':
            a = QUIT;
            break;
        case '|':
            a = SWITCH_PLAYER;
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
    ACTION a = key_press_to_action(c);
    switch (a) {
        case UP:
        case RIGHT:
        case DOWN:
        case LEFT:
            perform_move(a, current_player);
            break;
        case PLACE_BOMB:
            // TODO
            break;
        case NONE:
            break;
        case CHAT_WRITE:
            add_to_line(c);
            break;
        case CHAT_ERASE:
            decrement_line();
            break;
        case QUIT:
            return true;
        case SWITCH_PLAYER:
            current_player = (current_player + 1) % PLAYER_NUM;
    }
    return false;
}

int init_game() {
    init_view();
    init_controller();

    dimension dim;
    get_width_height_terminal(&dim);
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
