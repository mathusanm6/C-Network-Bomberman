#include "./view.h"

#include <ncurses.h>

void init_view() {
    initscr();     /* Start curses mode */
    raw();         /* Disable line buffering */
    noecho();      /* Don't echo() while we do getch (we will manually print characters when relevant) */
    curs_set(0);   // Set the cursor to invisible
    start_color(); // Enable colors
    init_pair(1, COLOR_YELLOW, COLOR_BLACK); // Define a new color style (text is yellow, background is black)
}

void end_view() {
    curs_set(1); // Set the cursor to visible again
    endwin();    /* End curses mode */
}

void get_width_height_terminal(int *width, int *height) {
    if (width != NULL && height != NULL) {
        getmaxyx(stdscr, *width, *height);
    }
}

void refresh_game(board *b, line *l) {
    // Update grid
    int x, y;
    for (y = 0; y < b->height; y++) {
        for (x = 0; x < b->width; x++) {
            char c;
            switch (get_grid(x, y)) {
                case 0:
                    c = ' ';
                    break;
                case 1:
                    c = 'O';
                    break;
                default:
                    c = '?';
                    break;
            }
            mvaddch(y + 1, x + 1, c);
        }
    }
    for (x = 0; x < b->width + 2; x++) {
        mvaddch(0, x, '-');
        mvaddch(b->height + 1, x, '-');
    }
    for (y = 0; y < b->height + 2; y++) {
        mvaddch(y, 0, '|');
        mvaddch(y, b->width + 1, '|');
    }
    // Update chat text
    attron(COLOR_PAIR(1)); // Enable custom color 1
    attron(A_BOLD);        // Enable bold
    for (x = 0; x < b->width + 2; x++) {
        if (x >= TEXT_SIZE || x >= l->cursor) {
            mvaddch(b->height + 2, x, ' ');
        } else {
            mvaddch(b->height + 2, x, l->data[x]);
        }
    }
    attroff(A_BOLD);        // Disable bold
    attroff(COLOR_PAIR(1)); // Disable custom color 1
    refresh();              // Apply the changes to the terminal
}
