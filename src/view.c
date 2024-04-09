#include "./view.h"

#include <stdlib.h>

#include "utils.h"

typedef struct window_context {
    dimension dim;
    int start_y;
    int start_x;
    WINDOW *win;
} window_context;

typedef struct padding {
    int top;
    int left;
} padding; // Padding right and bottom are not needed (because of inferring)

static window_context *game_wc;
static window_context *chat_wc;
static window_context *chat_history_wc;
static window_context *chat_input_wc;

static const padding PLAYABLE_PADDING = {PADDING_PLAYABLE_TOP, PADDING_PLAYABLE_LEFT};
static const padding SCREEN_PADDING = {PADDING_SCREEN_TOP, PADDING_SCREEN_LEFT};

static int validate_terminal_size();

// Helper functions for managing windows
static int init_windows();
static void del_window(window_context *);
static void del_all_windows();

// Helper functions for splitting the terminal window
static int split_terminal_window(window_context *, window_context *);
static int split_chat_window(window_context *, window_context *, window_context *);

// Helper functions for refreshing the game and chat windows
static void print_game(board *, window_context *);
static void print_chat(line *, window_context *, window_context *);

int init_colors() {
    // TODO : Might be an issue for university computers
    if (has_colors() == FALSE) {
        endwin();
        printf("Your terminal does not support color\n");
        return EXIT_FAILURE;
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

    return EXIT_SUCCESS;
}

int init_view() {
    initscr();   /* Start curses mode */
    raw();       /* Disable line buffering */
    noecho();    /* Don't echo() while we do getch (we will manually print characters when relevant) */
    curs_set(0); // Set the cursor to invisible

    if (init_colors() < 0) { // Initialize the colors
        return EXIT_FAILURE;
    }

    if (validate_terminal_size() < 0) { // Check if the terminal is big enough
        return EXIT_FAILURE;
    }

    if (init_windows() < 0) { // Initialize the windows
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
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
        dim->height = min(scr_dim.height, scr_dim.width); // 2/3 of the terminal height is customizable
        dim->width = dim->height * 2;                     // 2:1 aspect ratio (ncurses characters are not square)
    }
}

void add_padding(dimension *dim, padding pad) {
    if (dim != NULL) {
        dim->height -= 2 * pad.top;
        dim->width -= 2 * pad.left;
    }
}

void get_computed_board_dimension(dimension *dim) {
    if (dim != NULL) {
        dimension scr_dim;
        get_height_width_terminal(&scr_dim);
        get_height_width_playable(dim, scr_dim);
        add_padding(dim, PLAYABLE_PADDING);
        add_padding(dim, SCREEN_PADDING);
    }
}

void refresh_game(board *b, line *l) {

    print_game(b, game_wc);
    wrefresh(game_wc->win); // Refresh the game window

    print_chat(l, chat_history_wc, chat_input_wc);
    wrefresh(chat_input_wc->win);   // Refresh the chat input window (Before chat_win refresh)
    wrefresh(chat_history_wc->win); // Refresh the chat history window
    wrefresh(chat_wc->win);         // Refresh the chat window

    refresh(); // Apply the changes to the terminal
}

int validate_terminal_size() {
    dimension dim;
    get_height_width_terminal(&dim);

    if (dim.height < MIN_GAME_HEIGHT || dim.width < MIN_GAME_WIDTH) {
        end_view();
        printf("Please resize your terminal to have at least %d rows and %d columns and restart the game.\n",
               MIN_GAME_HEIGHT, MIN_GAME_WIDTH);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int init_windows() {
    game_wc = malloc(sizeof(window_context));
    chat_wc = malloc(sizeof(window_context));
    chat_history_wc = malloc(sizeof(window_context));
    chat_input_wc = malloc(sizeof(window_context));

    if (split_terminal_window(game_wc, chat_wc) < 0)
        return EXIT_FAILURE;

    if (split_chat_window(chat_wc, chat_history_wc, chat_input_wc) < 0)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

void del_all_windows() {
    del_window(game_wc);
    del_window(chat_history_wc);
    del_window(chat_input_wc);
    del_window(chat_wc);
}

void del_window(window_context *wc) {
    if (wc == NULL || wc->win == NULL) {
        return;
    }
    delwin(wc->win);
    wc->win = NULL;
    free(wc);
}

int split_terminal_window(window_context *game_wc, window_context *chat_wc) {
    dimension scr_dim;
    get_height_width_terminal(&scr_dim);
    padding pad = {PADDING_SCREEN_TOP, PADDING_SCREEN_LEFT};
    add_padding(&scr_dim, pad);
    // Game window is a square occupying maximum right side of the terminal
    dimension game_dim;
    get_height_width_playable(&game_dim, scr_dim);
    game_wc->dim.height = game_dim.height;
    game_wc->dim.width = game_dim.width;
    game_wc->start_y = pad.top + (scr_dim.height - game_wc->dim.height) / 2; // Center the window vertically
    game_wc->start_x = pad.left;
    game_wc->win = newwin(game_wc->dim.height, game_wc->dim.width, game_wc->start_y, game_wc->start_x);

    if (game_wc->win == NULL) {
        end_view();
        printf("Error creating game window\n");
        return EXIT_FAILURE;
    }

    chat_wc->dim.height = scr_dim.height;
    chat_wc->dim.width = scr_dim.width - game_wc->dim.width;
    chat_wc->start_y = pad.top;
    chat_wc->start_x = game_wc->dim.width + pad.left;
    chat_wc->win = newwin(chat_wc->dim.height, chat_wc->dim.width, chat_wc->start_y, chat_wc->start_x);

    if (chat_wc->win == NULL) {
        end_view();
        printf("Error creating chat window\n");
        return EXIT_FAILURE;
    }

    // Border around the windows
    box(game_wc->win, 0, 0);
    box(chat_wc->win, 0, 0);

    // Apply color to the borders
    wbkgd(game_wc->win, COLOR_PAIR(1));
    wbkgd(chat_wc->win, COLOR_PAIR(2));

    return EXIT_SUCCESS;
}

int split_chat_window(window_context *chat_wc, window_context *chat_history_wc, window_context *chat_input_wc) {

    chat_history_wc->dim.height = chat_wc->dim.height - 3;
    chat_history_wc->dim.width = chat_wc->dim.width;
    chat_history_wc->start_y = chat_wc->start_y;
    chat_history_wc->start_x = chat_wc->start_x;
    chat_history_wc->win = subwin(chat_wc->win, chat_history_wc->dim.height, chat_history_wc->dim.width,
                                  chat_history_wc->start_y, chat_history_wc->start_x);

    if (chat_history_wc->win == NULL) {
        end_view();
        printf("Error creating sub chat history window\n");
        return EXIT_FAILURE;
    }

    chat_input_wc->dim.height = 3;
    chat_input_wc->dim.width = chat_wc->dim.width;
    chat_input_wc->start_y = chat_wc->start_y + chat_wc->dim.height - 3;
    chat_input_wc->start_x = chat_wc->start_x;
    chat_input_wc->win = subwin(chat_wc->win, chat_input_wc->dim.height, chat_input_wc->dim.width,
                                chat_input_wc->start_y, chat_input_wc->start_x);

    if (chat_input_wc->win == NULL) {
        end_view();
        printf("Error creating sub chat input window\n");
        return EXIT_FAILURE;
    }

    scrollok(chat_history_wc->win, TRUE); // Enable scrolling in the chat history window

    box(chat_history_wc->win, 0, 0);
    box(chat_input_wc->win, 0, 0);

    wbkgd(chat_history_wc->win, COLOR_PAIR(3));
    wbkgd(chat_input_wc->win, COLOR_PAIR(3));

    return EXIT_SUCCESS;
}

void activate_color_for_tile(window_context *wc, TILE tile) {
    switch (tile) {
        case PLAYER_1:
            wattron(wc->win, COLOR_PAIR(4));
            break;
        case PLAYER_2:
            wattron(wc->win, COLOR_PAIR(5));
            break;
        case PLAYER_3:
            wattron(wc->win, COLOR_PAIR(6));
            break;
        case PLAYER_4:
            wattron(wc->win, COLOR_PAIR(7));
            break;
        case VERTICAL_BORDER:
        case HORIZONTAL_BORDER:
            wattron(wc->win, COLOR_PAIR(8));
            break;
        case INDESTRUCTIBLE_WALL:
            wattron(wc->win, COLOR_PAIR(9));
            break;
        case DESTRUCTIBLE_WALL:
            wattron(wc->win, COLOR_PAIR(10));
            break;
        case EMPTY:
            break;
        case BOMB:
            break;
        case EXPLOSION:
            break;
    }
}

void deactivate_color_for_tile(window_context *wc, TILE tile) {
    switch (tile) {
        case PLAYER_1:
            wattroff(wc->win, COLOR_PAIR(4));
            break;
        case PLAYER_2:
            wattroff(wc->win, COLOR_PAIR(5));
            break;
        case PLAYER_3:
            wattroff(wc->win, COLOR_PAIR(6));
            break;
        case PLAYER_4:
            wattroff(wc->win, COLOR_PAIR(7));
            break;
        case VERTICAL_BORDER:
        case HORIZONTAL_BORDER:
            wattroff(wc->win, COLOR_PAIR(8));
            break;
        case INDESTRUCTIBLE_WALL:
            wattroff(wc->win, COLOR_PAIR(9));
            break;
        case DESTRUCTIBLE_WALL:
            wattroff(wc->win, COLOR_PAIR(10));
            break;
        case EMPTY:
            break;
        case BOMB:
            break;
        case EXPLOSION:
            break;
    }
}

void print_game(board *b, window_context *game_wc) {
    // Update grid
    int x, y;
    dimension dim = b->dim;
    int pad_top =
        (game_wc->dim.height - dim.height - 2) / 2; // We can substract 2 or 1 but the first enable a left align
    int pad_left = (game_wc->dim.width - dim.width - 2) / 2; // whether 1 is for a right align
    padding pad = {pad_top, pad_left};
    char vb = tile_to_char(VERTICAL_BORDER);
    char hb = tile_to_char(HORIZONTAL_BORDER);
    for (y = 0; y < b->dim.height; y++) {
        for (x = 0; x < b->dim.width; x++) {
            TILE t = get_grid(x, y);
            char c = tile_to_char(t);
            activate_color_for_tile(game_wc, t);
            mvwaddch(game_wc->win, y + 1 + pad.top, x + 1 + pad.left, c);
            deactivate_color_for_tile(game_wc, t);
        }
    }

    activate_color_for_tile(game_wc, hb);
    for (x = 0; x < b->dim.width + 2; x++) {
        mvwaddch(game_wc->win, pad.top, x + pad.left, hb);
        mvwaddch(game_wc->win, b->dim.height + 1 + pad.top, x + pad.left, hb);
    }
    deactivate_color_for_tile(game_wc, hb);

    activate_color_for_tile(game_wc, vb);
    for (y = 0; y < b->dim.height + 2; y++) {
        mvwaddch(game_wc->win, y + pad.top, pad.left, vb);
        mvwaddch(game_wc->win, y + pad.top, b->dim.width + 1 + pad.left, vb);
    }
    deactivate_color_for_tile(game_wc, vb);
}

void print_chat(line *l, window_context *chat_history_wc, window_context *chat_input_wc) {
    // Update chat text
    wattron(chat_input_wc->win, COLOR_PAIR(3)); // Enable custom color 3
    wattron(chat_input_wc->win, A_BOLD);        // Enable bold
    int x;
    char e = tile_to_char(EMPTY);
    for (x = 0; x < chat_input_wc->dim.width - 2; x++) {
        if (x >= TEXT_SIZE || x >= l->cursor) {
            mvwaddch(chat_input_wc->win, 1, x + 1, e);
        } else {
            mvwaddch(chat_input_wc->win, 1, x + 1, l->data[x]);
        }
    }
    wattroff(chat_input_wc->win, A_BOLD);        // Disable bold
    wattroff(chat_input_wc->win, COLOR_PAIR(3)); // Disable custom color 3

    // Update chat history
    wattron(chat_history_wc->win, COLOR_PAIR(3)); // Enable custom color 3
    // TODO: Implement chat history
    wattroff(chat_history_wc->win, COLOR_PAIR(3)); // Disable custom color 3
}