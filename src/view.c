#include "./view.h"
#include "model.h"

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
    char vb = get_tile_into_char(VERTICAL_BORDER);
    char hb = get_tile_into_char(HORIZONTAL_BORDER);
    char e = get_tile_into_char(EMPTY);
    for (y = 0; y < b->height; y++) {
        for (x = 0; x < b->width; x++) {
            char c;
            c = get_tile_into_char(get_grid(x, y));
            mvaddch(y + 1, x + 1, c);
        }
    }
    for (x = 0; x < b->width + 2; x++) {
        mvaddch(0, x, hb);
        mvaddch(b->height + 1, x, hb);
    }
    for (y = 0; y < b->height + 2; y++) {
        mvaddch(y, 0, vb);
        mvaddch(y, b->width + 1, vb);
    }
    // Update chat text
    attron(COLOR_PAIR(1)); // Enable custom color 1
    attron(A_BOLD);        // Enable bold
    for (x = 0; x < b->width + 2; x++) {
        if (x >= TEXT_SIZE || x >= l->cursor) {
            mvaddch(b->height + 2, x, e);
        } else {
            mvaddch(b->height + 2, x, l->data[x]);
        }
    }
    attroff(A_BOLD);        // Disable bold
    attroff(COLOR_PAIR(1)); // Disable custom color 1
    refresh();              // Apply the changes to the terminal
}
