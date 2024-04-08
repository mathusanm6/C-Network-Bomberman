#include "./view.h"

#include <stdlib.h>

#include "utils.h"

window *game_win;
window *chat_win;
window *chat_history_win;
window *chat_input_win;

void validate_terminal_size();

// Helper functions for managing windows
void init_windows();
void del_window(window *);
void del_all_windows();

// Helper functions for splitting the terminal window
void split_terminal_window(window *, window *);
void split_chat_window(window *, window *, window *);

// Helper functions for refreshing the game and chat windows
void print_game(board *, window *);
void print_chat(line *, window *, window *);

void init_colors() {
    if (has_colors() == FALSE) {
        endwin();
        printf("Your terminal does not support color\n");
        exit(1);
    }

    start_color();                           // Enable colors
    init_pair(1, COLOR_WHITE, COLOR_BLACK);  // For the game window
    init_pair(2, COLOR_GREEN, COLOR_BLACK);  // For the chat window
    init_pair(3, COLOR_YELLOW, COLOR_BLACK); // For the chat box

    // Initialize the colors for the players
    init_pair(4, COLOR_RED, COLOR_BLACK);
    init_pair(5, COLOR_GREEN, COLOR_BLACK);
    init_pair(6, COLOR_YELLOW, COLOR_BLACK);
    init_pair(7, COLOR_WHITE, COLOR_BLACK);

    // Initialize the colors for the borders
    init_pair(8, COLOR_WHITE, COLOR_BLACK);

    // Initialize the colors for indestructible walls
    init_pair(9, COLOR_MAGENTA, COLOR_BLACK);

    // Initialize the colors for destructible walls
    init_pair(10, COLOR_BLUE, COLOR_BLACK);
}

void init_view() {
    initscr();                /* Start curses mode */
    raw();                    /* Disable line buffering */
    noecho();                 /* Don't echo() while we do getch (we will manually print characters when relevant) */
    curs_set(0);              // Set the cursor to invisible
    init_colors();            // Initialize the colors
    validate_terminal_size(); // Check if the terminal is big enough
    init_windows();           // Initialize the windows
}

void end_view() {
    del_all_windows(); // Delete all the windows and free the memory
    curs_set(1);       // Set the cursor to visible again
    endwin();          /* End curses mode */
}

void get_height_width_terminal(dimension *dim) {
    if (dim != NULL) {
        getmaxyx(stdscr, dim->height, dim->width);
    }
}

void get_height_width_playable(dimension *dim, dimension scr_dim) {
    if (dim != NULL) {
        dim->height = min((scr_dim.height / 3) * 2, scr_dim.width); // 2/3 of the terminal height is customizable
        dim->width = dim->height * 2; // 2:1 aspect ratio (ncurses characters are not square)
    }
}

void add_padding(dimension *dim, padding pad) {
    if (dim != NULL) {
        dim->height -= 2 * pad.top;
        dim->width -= 2 * pad.left;
    }
}

void refresh_game(board *b, line *l) {

    print_game(b, game_win);
    wrefresh(game_win->win); // Refresh the game window

    print_chat(l, chat_history_win, chat_input_win);
    wrefresh(chat_input_win->win);   // Refresh the chat input window (Before chat_win refresh)
    wrefresh(chat_history_win->win); // Refresh the chat history window
    wrefresh(chat_win->win);         // Refresh the chat window

    refresh(); // Apply the changes to the terminal
}

void validate_terminal_size() {
    dimension dim;
    get_height_width_terminal(&dim);

    if (dim.height < MIN_GAME_HEIGHT || dim.width < MIN_GAME_WIDTH) {
        end_view();
        printf("Please resize your terminal to at least %d rows and %d columns and restart the game.\n",
               MIN_GAME_HEIGHT, MIN_GAME_WIDTH);
        exit(1);
    }
}

void init_windows() {
    game_win = malloc(sizeof(window));
    chat_win = malloc(sizeof(window));
    chat_history_win = malloc(sizeof(window));
    chat_input_win = malloc(sizeof(window));

    split_terminal_window(game_win, chat_win);
    split_chat_window(chat_win, chat_history_win, chat_input_win);
}

void del_all_windows() {
    del_window(game_win);
    del_window(chat_history_win);
    del_window(chat_input_win);
    del_window(chat_win);
}

void del_window(window *win) {
    if (win == NULL || win->win == NULL) {
        return;
    }
    delwin(win->win);
    win->win = NULL;
    free(win);
}

void split_terminal_window(window *game_win, window *chat_win) {
    dimension scr_dim;
    get_height_width_terminal(&scr_dim);
    padding pad = {PADDING_SCREEN_TOP, PADDING_SCREEN_LEFT};
    add_padding(&scr_dim, pad);
    // Game window is a square occupying maximum right side of the terminal
    dimension game_dim;
    get_height_width_playable(&game_dim, scr_dim);
    game_win->dim.height = game_dim.height;
    game_win->dim.width = game_dim.width;
    game_win->start_y = pad.top + (scr_dim.height - game_win->dim.height) / 2; // Center the window vertically
    game_win->start_x = pad.left;
    game_win->win = newwin(game_win->dim.height, game_win->dim.width, game_win->start_y, game_win->start_x);

    if (game_win->win == NULL) {
        end_view();
        printf("Error creating game window\n");
        exit(1);
    }

    chat_win->dim.height = scr_dim.height;
    chat_win->dim.width = scr_dim.width - game_win->dim.width;
    chat_win->start_y = pad.top;
    chat_win->start_x = game_win->dim.width + pad.left;
    chat_win->win = newwin(chat_win->dim.height, chat_win->dim.width, chat_win->start_y, chat_win->start_x);

    if (chat_win->win == NULL) {
        end_view();
        printf("Error creating chat window\n");
        exit(1);
    }

    // Border around the windows
    box(game_win->win, 0, 0);
    box(chat_win->win, 0, 0);

    // Apply color to the borders
    wbkgd(game_win->win, COLOR_PAIR(1));
    wbkgd(chat_win->win, COLOR_PAIR(2));
}

void split_chat_window(window *chat_win, window *chat_history_win, window *chat_input_win) {

    chat_history_win->dim.height = chat_win->dim.height - 3;
    chat_history_win->dim.width = chat_win->dim.width;
    chat_history_win->start_y = chat_win->start_y;
    chat_history_win->start_x = chat_win->start_x;
    chat_history_win->win = subwin(chat_win->win, chat_history_win->dim.height, chat_history_win->dim.width,
                                   chat_history_win->start_y, chat_history_win->start_x);

    if (chat_history_win->win == NULL) {
        end_view();
        printf("Error creating sub chat history window\n");
        exit(1);
    }

    chat_input_win->dim.height = 3;
    chat_input_win->dim.width = chat_win->dim.width;
    chat_input_win->start_y = chat_win->start_y + chat_win->dim.height - 3;
    chat_input_win->start_x = chat_win->start_x;
    chat_input_win->win = subwin(chat_win->win, chat_input_win->dim.height, chat_input_win->dim.width,
                                 chat_input_win->start_y, chat_input_win->start_x);

    if (chat_input_win->win == NULL) {
        end_view();
        printf("Error creating sub chat input window\n");
        exit(1);
    }

    scrollok(chat_history_win->win, TRUE); // Enable scrolling in the chat history window

    box(chat_history_win->win, 0, 0);
    box(chat_input_win->win, 0, 0);

    wbkgd(chat_history_win->win, COLOR_PAIR(3));
    wbkgd(chat_input_win->win, COLOR_PAIR(3));
}

void activate_color_for_tile(window *win_, TILE tile) {
    switch (tile) {
        case PLAYER_1:
            wattron(win_->win, COLOR_PAIR(4));
            break;
        case PLAYER_2:
            wattron(win_->win, COLOR_PAIR(5));
            break;
        case PLAYER_3:
            wattron(win_->win, COLOR_PAIR(6));
            break;
        case PLAYER_4:
            wattron(win_->win, COLOR_PAIR(7));
            break;
        case VERTICAL_BORDER:
        case HORIZONTAL_BORDER:
            wattron(win_->win, COLOR_PAIR(8));
            break;
        case INDESTRUCTIBLE_WALL:
            wattron(win_->win, COLOR_PAIR(9));
            break;
        case DESTRUCTIBLE_WALL:
            wattron(win_->win, COLOR_PAIR(10));
            break;
        case EMPTY:
            break;
        case BOMB:
            break;
        case EXPLOSION:
            break;
    }
}

void deactivate_color_for_tile(window *win, TILE tile) {
    switch (tile) {
        case PLAYER_1:
            wattroff(win->win, COLOR_PAIR(4));
            break;
        case PLAYER_2:
            wattroff(win->win, COLOR_PAIR(5));
            break;
        case PLAYER_3:
            wattroff(win->win, COLOR_PAIR(6));
            break;
        case PLAYER_4:
            wattroff(win->win, COLOR_PAIR(7));
            break;
        case VERTICAL_BORDER:
        case HORIZONTAL_BORDER:
            wattroff(win->win, COLOR_PAIR(8));
            break;
        case INDESTRUCTIBLE_WALL:
            wattroff(win->win, COLOR_PAIR(9));
            break;
        case DESTRUCTIBLE_WALL:
            wattroff(win->win, COLOR_PAIR(10));
            break;
        case EMPTY:
            break;
        case BOMB:
            break;
        case EXPLOSION:
            break;
    }
}

void print_game(board *b, window *game_win) {
    // Update grid
    int x, y;
    dimension dim = b->dim;
    if (dim.width % 2 == 0) { // The game_board width has to be odd to fill it with content
        dim.width--;
    }
    if (dim.height % 2 == 1) { // The game board height has to be even to fill it with content
        dim.height--;
    }
    padding pad = {(game_win->dim.height - dim.height - 1) / 2, (game_win->dim.width - dim.width - 1) / 2};
    char vb = tile_to_char(VERTICAL_BORDER);
    char hb = tile_to_char(HORIZONTAL_BORDER);
    for (y = 0; y < b->dim.height; y++) {
        for (x = 0; x < b->dim.width; x++) {
            TILE t = get_grid(x, y);
            char c = tile_to_char(t);
            activate_color_for_tile(game_win, t);
            mvwaddch(game_win->win, y + 1 + pad.top, x + 1 + pad.left, c);
            deactivate_color_for_tile(game_win, t);
        }
    }

    activate_color_for_tile(game_win, hb);
    for (x = 0; x < b->dim.width + 2; x++) {
        mvwaddch(game_win->win, pad.top, x + pad.left, hb);
        mvwaddch(game_win->win, b->dim.height + 1 + pad.top, x + pad.left, hb);
    }
    deactivate_color_for_tile(game_win, hb);

    activate_color_for_tile(game_win, vb);
    for (y = 0; y < b->dim.height + 2; y++) {
        mvwaddch(game_win->win, y + pad.top, pad.left, vb);
        mvwaddch(game_win->win, y + pad.top, b->dim.width + 1 + pad.left, vb);
    }
    deactivate_color_for_tile(game_win, vb);
}

void print_chat(line *l, window *chat_history_win, window *chat_input_win) {
    // Update chat text
    wattron(chat_input_win->win, COLOR_PAIR(3)); // Enable custom color 3
    wattron(chat_input_win->win, A_BOLD);        // Enable bold
    int x;
    char e = tile_to_char(EMPTY);
    for (x = 1; x < chat_input_win->dim.width - 1; x++) {
        if (x >= TEXT_SIZE || x >= l->cursor) {
            mvwaddch(chat_input_win->win, 1, x, e);
        } else {
            mvwaddch(chat_input_win->win, 1, x, l->data[x]);
        }
    }
    wattroff(chat_input_win->win, A_BOLD);        // Disable bold
    wattroff(chat_input_win->win, COLOR_PAIR(3)); // Disable custom color 3

    // Update chat history
    wattron(chat_history_win->win, COLOR_PAIR(3)); // Enable custom color 3
    // TODO: Implement chat history
    wattroff(chat_history_win->win, COLOR_PAIR(3)); // Disable custom color 3
}