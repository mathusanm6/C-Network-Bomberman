#include "./model.h"

#include <stdio.h>
#include <stdlib.h>

board *game_board = NULL;
line *chat_line = NULL;
coord *current_pos = NULL;

int init_game_board(int width, int height) {
    if (game_board == NULL) {
        game_board = malloc(sizeof(board));
        if (game_board == NULL) {
            perror("malloc");
            return EXIT_FAILURE;
        }
        game_board->height = width - 2 - 1; // 2 rows reserved for border, 1 row for chat
        game_board->width = height - 2;     // 2 columns reserved for border
        game_board->grid = calloc((game_board->width) * (game_board->height), sizeof(char));
        if (game_board->grid == NULL) {
            perror("calloc");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

int init_chat_line() {
    if (chat_line == NULL) {
        chat_line = malloc(sizeof(line));
        if (chat_line == NULL) {
            perror("malloc");
            return EXIT_FAILURE;
        }
        chat_line->cursor = 0;
    }
    return EXIT_SUCCESS;
}

int init_current_position() {
    if (current_pos == NULL) {
        current_pos = malloc(sizeof(coord));
        if (current_pos == NULL) {
            perror("malloc");
            return EXIT_FAILURE;
        }
        current_pos->x = 0;
        current_pos->y = 0;
    }
    return EXIT_SUCCESS;
}

int init_model(int width, int height) {
    if (init_game_board(width, height) < 0) {
        return EXIT_FAILURE;
    } else if (init_chat_line() < 0) {
        return EXIT_FAILURE;
    } else if (init_current_position() < 0) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void free_game_board() {
    if (game_board != NULL) {
        if (game_board->grid != NULL) {
            free(game_board->grid);
            game_board->grid = NULL;
        }
        free(game_board);
        game_board = NULL;
    }
}

void free_chat_line() {
    if (chat_line != NULL) {
        free(chat_line);
        chat_line = NULL;
    }
}

void free_current_position() {
    if (current_pos != NULL) {
        free(current_pos);
        current_pos = NULL;
    }
}

void free_model() {
    free_game_board();
    free_chat_line();
    free_current_position();
}

char tile_to_char(TILE t) {
    char c;
    switch (t) {
        case EMPTY:
            c = ' ';
            break;
        case INDESTRUCTIBLE_WALL:
            c = '#';
            break;
        case DESTRUCTIBLE_WALL:
            c = '@';
            break;
        case BOMB:
            c = 'o';
            break;
        case EXPLOSION:
            c = '+';
            break;
        case PLAYER_1:
            c = '1';
            break;
        case PLAYER_2:
            c = '2';
            break;
        case PLAYER_3:
            c = '3';
            break;
        case PLAYER_4:
            c = '4';
            break;
        case VERTICAL_BORDER:
            c = '|';
            break;
        case HORIZONTAL_BORDER:
            c = '-';
            break;
    }
    return c;
}

coord int_to_coord(int n) {
    coord c;
    c.y = n / game_board->width;
    c.x = n % game_board->width;
    return c;
}

int coord_to_int(int x, int y) {
    return y * game_board->width + x;
}

TILE get_grid(int x, int y) {
    if (game_board != NULL) {
        return game_board->grid[coord_to_int(x, y)];
    }
    return EXIT_FAILURE;
}

void set_grid(int x, int y, TILE v) {
    if (game_board != NULL) {
        game_board->grid[coord_to_int(x, y)] = v;
    }
}

void decrement_line() {
    if (chat_line != NULL && chat_line->cursor > 0) {
        chat_line->cursor--;
    }
}

void add_to_line(char c) {
    if (chat_line != NULL && chat_line->cursor < TEXT_SIZE && c >= ' ' && c <= '~') {
        chat_line->data[(chat_line->cursor)] = c;
        (chat_line->cursor)++;
    }
}

void perform_move(ACTION a) {
    int dx = 0;
    int dy = 0;
    switch (a) {
        case LEFT:
            dx = -1;
            dy = 0;
            break;
        case RIGHT:
            dx = 1;
            dy = 0;
            break;
        case UP:
            dx = 0;
            dy = -1;
            break;
        case DOWN:
            dx = 0;
            dy = 1;
            break;
        default:
            break;
    }
    current_pos->x += dx;
    current_pos->y += dy;
    current_pos->x = (current_pos->x + game_board->width) % game_board->width;
    current_pos->y = (current_pos->y + game_board->height) % game_board->height;
    set_grid(current_pos->x, current_pos->y, PLAYER_1);
}
