#include "./model.h"

#include <stdio.h>
#include <stdlib.h>

line *chat_line = NULL;

static board *game_board = NULL;
static coord *player_positions[PLAYER_NUM] = {NULL, NULL, NULL, NULL};

int init_game_board(int width, int height) {
    if (game_board == NULL) {
        game_board = malloc(sizeof(board));
        if (game_board == NULL) {
            perror("malloc");
            return EXIT_FAILURE;
        }
        game_board->width = width - 2 - 1; // 2 rows reserved for border, 1 row for chat
        game_board->height = height - 2;   // 2 columns reserved for border
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

int init_player_positions() {
    for (int i = 0; i < 4; i++) {
        player_positions[i] = malloc(sizeof(coord));
        if (player_positions[i] == NULL) {
            perror("malloc");
            return EXIT_FAILURE;
        }
        player_positions[i]->x = 0;
        player_positions[i]->y = 0;
    }
    return EXIT_SUCCESS;
}

int init_model(int width, int height) {
    if (init_game_board(width, height) < 0) {
        return EXIT_FAILURE;
    } else if (init_chat_line() < 0) {
        return EXIT_FAILURE;
    } else if (init_player_positions() < 0) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void free_board(board *game_board) {
    if (game_board != NULL) {
        if (game_board->grid != NULL) {
            free(game_board->grid);
            game_board->grid = NULL;
        }
        free(game_board);
        game_board = NULL;
    }
}

void free_game_board() {
    free_board(game_board);
}

void free_chat_line() {
    if (chat_line != NULL) {
        free(chat_line);
        chat_line = NULL;
    }
}

void free_player_positions() {
    for (int i = 0; i < 4; i++) {
        if (player_positions[i] != NULL) {
            free(player_positions[i]);
            player_positions[i] = NULL;
        }
    }
}

void free_model() {
    free_game_board();
    free_chat_line();
    free_player_positions();
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

TILE get_player(int player_id) {
    switch (player_id) {
        case 0:
            return PLAYER_1;
        case 1:
            return PLAYER_2;
        case 2:
            return PLAYER_3;
        case 3:
            return PLAYER_4;
        default:
            return EMPTY;
    }
}

void perform_move(ACTION a, int player_id) {
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

    coord *current_pos = player_positions[player_id];

    current_pos->x += dx;
    current_pos->y += dy;
    current_pos->x = (current_pos->x + game_board->width) % game_board->width;
    current_pos->y = (current_pos->y + game_board->height) % game_board->height;
    set_grid(current_pos->x, current_pos->y, get_player(player_id));
}

board *get_game_board() {
    board *copy = malloc(sizeof(board));
    if (copy == NULL) {
        perror("malloc");
        return NULL;
    }

    copy->width = game_board->width;
    copy->height = game_board->height;
    copy->grid = malloc(sizeof(char) * copy->width * copy->height);
    if (copy->grid == NULL) {
        perror("malloc");
        free(copy);
        return NULL;
    }

    for (int i = 0; i < copy->width * copy->height; i++) {
        copy->grid[i] = game_board->grid[i];
    }

    return copy;
}
