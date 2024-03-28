#include "./controller.h"
#include "./model.h"
#include "./view.h"

#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>

void init_controller() {
    intrflush(stdscr, FALSE); /* No need to flush when intr key is pressed */
    keypad(stdscr, TRUE);     /* Required in order to get events from keyboard */
    nodelay(stdscr, TRUE);    /* Make getch non-blocking */
}

ACTION get_action_with_control(int c) {
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
    ACTION a = get_action_with_control(c);
    switch (a) {
        case UP:
        case RIGHT:
        case DOWN:
        case LEFT:
            perform_move(a);
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
    }
    return false;
}

int init_game() {
    init_view();
    init_controller();

    int width, height;
    get_width_height_terminal(&width, &height);
    if (init_model(width, height) < 0) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int game_loop() {
    while (true) {
        if (control()) {
            break;
        }
        refresh_game(game_board, chat_line);
        usleep(30 * 1000);
    }
    free_model();
    end_view();

    return EXIT_SUCCESS;
}
