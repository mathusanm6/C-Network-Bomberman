#include "./model.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

line *chat_line = NULL;

static board *game_board = NULL;
static coord *player_positions[PLAYER_NUM] = {NULL, NULL, NULL, NULL};

TILE get_player(int);

TILE get_probably_destructible_wall() {
    if (random() % DESTRUCTIBLE_WALL_CHANCE == 0) {
        return EMPTY;
    }
    return DESTRUCTIBLE_WALL;
}

int init_game_board_content() {
    if (game_board == NULL) {
        return EXIT_FAILURE;
    }
    srandom(time(NULL));

    // Indestructible wall part
    for (int c = 1; c < game_board->dim.width - 1; c += 2) {
        for (int l = 1; l < game_board->dim.height - 1; l += 2) {
            game_board->grid[coord_to_int(c, l)] = INDESTRUCTIBLE_WALL;
        }
    }

    // Destructible wall part
    for (int c = 2; c < game_board->dim.width - 2; c++) {
        game_board->grid[coord_to_int(c, 0)] = get_probably_destructible_wall();
        game_board->grid[coord_to_int(c, game_board->dim.height - 1)] = get_probably_destructible_wall();
    }
    for (int c = 2; c < game_board->dim.width - 2; c += 2) {
        game_board->grid[coord_to_int(c, 1)] = get_probably_destructible_wall();
        game_board->grid[coord_to_int(c, game_board->dim.height - 2)] = get_probably_destructible_wall();
    }
    for (int l = 2; l < game_board->dim.height - 2; l++) {
        if (l % 2 == 0) {
            for (int c = 0; c < game_board->dim.width; c++) {
                game_board->grid[coord_to_int(c, l)] = get_probably_destructible_wall();
            }
        } else {
            for (int c = 0; c < game_board->dim.width; c += 2) {
                game_board->grid[coord_to_int(c, l)] = get_probably_destructible_wall();
            }
        }
    }
    return EXIT_SUCCESS;
}

int init_game_board(dimension dim) {
    if (dim.width % 2 == 0) { // The game_board width has to be odd to fill it with content
        dim.width--;
    }
    if (dim.height % 2 == 1) { // The game board height has to be even to fill it with content
        dim.height--;
    }
    if (dim.width < MIN_GAMEBOARD_WIDTH || dim.height < MIN_GAMEBOARD_HEIGHT) {
        return EXIT_FAILURE;
    }
    if (game_board == NULL) {
        game_board = malloc(sizeof(board));
        if (game_board == NULL) {
            perror("malloc");
            return EXIT_FAILURE;
        }
        game_board->dim.height = dim.height - 2 - 1; // 2 rows reserved for border, 1 row for chat
        game_board->dim.width = dim.width - 2;       // 2 columns reserved for border
        game_board->grid = calloc((game_board->dim.width) * (game_board->dim.height), sizeof(char));
        if (game_board->grid == NULL) {
            perror("calloc");
            return EXIT_FAILURE;
        }
    }
    if (init_game_board_content() == EXIT_FAILURE) {
        free_board(game_board);
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
        if (i < 2) {
            player_positions[i]->y = 0;
        } else {
            player_positions[i]->y = game_board->dim.height - 1;
        }
        player_positions[i]->x = (game_board->dim.width - (i % 2)) % game_board->dim.width;
        set_grid(player_positions[i]->x, player_positions[i]->y, get_player(i));
    }
    return EXIT_SUCCESS;
}

int init_model(dimension dim) {
    if (init_game_board(dim) < 0) {
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

bool is_outside_board(int x, int y) {
    return x < 0 || x >= game_board->dim.width || y < 0 || y >= game_board->dim.height;
}

bool is_wall_of_grid(int x, int y) {
    if (is_outside_board(x, y)) {
        return true;
    }
    TILE t = get_grid(x, y);
    return t == INDESTRUCTIBLE_WALL || t == DESTRUCTIBLE_WALL;
}

coord int_to_coord(int n) {
    coord c;
    c.y = n / game_board->dim.width;
    c.x = n % game_board->dim.width;
    return c;
}

int coord_to_int(int x, int y) {
    return y * game_board->dim.width + x;
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

coord get_next_position(ACTION a, const coord *pos) {
    coord c;
    c.x = pos->x;
    c.y = pos->y;
    switch (a) {
        case LEFT:
            c.x--;
            break;
        case RIGHT:
            c.x++;
            break;
        case UP:
            c.y--;
            break;
        case DOWN:
            c.y++;
            break;
        default:
            break;
    }
    return c;
}

void perform_move(ACTION a, int player_id) {
    coord *current_pos = player_positions[player_id];
    coord old_pos = *current_pos;

    coord c = get_next_position(a, current_pos);
    if (is_wall_of_grid(c.x, c.y)) {
        return;
    }
    current_pos->x = c.x;
    current_pos->y = c.y;
    set_grid(current_pos->x, current_pos->y, get_player(player_id));
    set_grid(old_pos.x, old_pos.y, EMPTY);
}

board *get_game_board() {
    board *copy = malloc(sizeof(board));
    if (copy == NULL) {
        perror("malloc");
        return NULL;
    }

    copy->dim.width = game_board->dim.width;
    copy->dim.height = game_board->dim.height;
    copy->grid = malloc(sizeof(char) * copy->dim.width * copy->dim.height);
    if (copy->grid == NULL) {
        perror("malloc");
        free(copy);
        return NULL;
    }

    for (int i = 0; i < copy->dim.width * copy->dim.height; i++) {
        copy->grid[i] = game_board->grid[i];
    }

    return copy;
}
