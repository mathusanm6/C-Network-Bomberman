#include "controller.h"
#include "model.h"
#include "view.h"

#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>

void init_controller() {
    intrflush(stdscr, FALSE); /* No need to flush when intr key is pressed */
    keypad(stdscr, TRUE);     /* Required in order to get events from keyboard */
    nodelay(stdscr, TRUE);    /* Make getch non-blocking */
}

ACTION control() {
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
    ACTION a = NONE;
    switch (prev_c) {
        case ERR:
            break;
        case KEY_LEFT:
            a = LEFT;
            break;
        case KEY_RIGHT:
            a = RIGHT;
            break;
        case KEY_UP:
            a = UP;
            break;
        case KEY_DOWN:
            a = DOWN;
            break;
        case '~':
            a = QUIT;
            break;
        case KEY_BACKSPACE:
            decrement_line();
            break;
        default:
            if (prev_c >= ' ' && prev_c <= '~')
                add_to_line(prev_c);
            break;
    }
    return a;
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

int game() {
    while (true) {
        ACTION action = control(chat_line);
        if (perform_action(action))
            break;
        refresh_game(game_board, chat_line);
        usleep(30 * 1000);
    }
    free_model();
    end_view();

    return EXIT_SUCCESS;
}
