#include "./model.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct bombs {
    bomb *list;
    int count;
    int capacity;
} bombs;

line *chat_line = NULL;

static board *game_board = NULL;
static bombs all_bombs = {NULL, 0, 0};
static coord *player_positions[PLAYER_NUM] = {NULL, NULL, NULL, NULL};
static GAME_MODE game_mode = SOLO;

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
    for (int c = 2; c < game_board->dim.width - 2; c++) { // Fill the first and last line
        game_board->grid[coord_to_int(c, 0)] = get_probably_destructible_wall();
        game_board->grid[coord_to_int(c, game_board->dim.height - 1)] = get_probably_destructible_wall();
    }
    for (int c = 2; c < game_board->dim.width - 2; c += 2) { // Fill the second and the second last line
        game_board->grid[coord_to_int(c, 1)] = get_probably_destructible_wall();
        game_board->grid[coord_to_int(c, game_board->dim.height - 2)] = get_probably_destructible_wall();
    }
    for (int l = 2; l < game_board->dim.height - 2; l++) { // Fill the other lines
        if (l % 2 == 0) { // There are no indestructible walls between destructible walls on this line
            for (int c = 0; c < game_board->dim.width; c++) {
                game_board->grid[coord_to_int(c, l)] = get_probably_destructible_wall();
            }
        } else { // There are indestructible walls between destructible walls on this line
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
    if (dim.height % 2 == 0) { // The game board height has to be odd to fill it with content
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
        game_board->dim.height = dim.height - 2; // 2 rows reserved for border
        game_board->dim.width = dim.width - 2;   // 2 columns reserved for border
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

int init_model(dimension dim, GAME_MODE game_mode_) {
    if (init_game_board(dim) == EXIT_FAILURE) {
        return EXIT_FAILURE;
    } else if (init_chat_line() == EXIT_FAILURE) {
        return EXIT_FAILURE;
    } else if (init_player_positions() == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }
    game_mode = game_mode_;
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

bool can_move_to_position(int x, int y) {
    if (is_outside_board(x, y)) {
        return false;
    }
    TILE t = get_grid(x, y);
    return t != BOMB && t != INDESTRUCTIBLE_WALL && t != DESTRUCTIBLE_WALL && t != PLAYER_1 && t != PLAYER_2 &&
           t != PLAYER_3 && t != PLAYER_4;
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

void clear_line() {
    if (chat_line != NULL) {
        chat_line->cursor = 0;
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

coord get_next_position(GAME_ACTION a, const coord *pos) {
    coord c;
    c.x = pos->x;
    c.y = pos->y;
    switch (a) {
        case GAME_LEFT:
            c.x--;
            break;
        case GAME_RIGHT:
            c.x++;
            break;
        case GAME_UP:
            c.y--;
            break;
        case GAME_DOWN:
            c.y++;
            break;
        default:
            break;
    }
    return c;
}

void perform_move(GAME_ACTION a, int player_id) {
    coord *current_pos = player_positions[player_id];
    coord old_pos = *current_pos;

    coord c = get_next_position(a, current_pos);
    if (!can_move_to_position(c.x, c.y)) {
        return;
    }
    current_pos->x = c.x;
    current_pos->y = c.y;
    set_grid(current_pos->x, current_pos->y, get_player(player_id));
    if (get_grid(old_pos.x, old_pos.y) != BOMB) {
        set_grid(old_pos.x, old_pos.y, EMPTY);
    }
}

void place_bomb(int player_id) {
    if (all_bombs.count == all_bombs.capacity) {
        int new_capacity = (all_bombs.capacity == 0) ? 4 : all_bombs.capacity * 2;
        bomb *new_list = realloc(all_bombs.list, new_capacity * sizeof(bomb));
        if (new_list == NULL) {
            perror("error realloc");
            return;
        }
        all_bombs.list = new_list;
        all_bombs.capacity = new_capacity;
    }

    coord *current_pos = player_positions[player_id];

    bomb new_bomb;
    new_bomb.pos.x = current_pos->x;
    new_bomb.pos.y = current_pos->y;
    new_bomb.placement_time = time(NULL);

    all_bombs.list[all_bombs.count] = new_bomb;
    all_bombs.count++;

    set_grid(current_pos->x, current_pos->y, BOMB);
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

GAME_MODE get_game_mode() {
    return game_mode;
}

void update_bombs() {
    time_t current_time = time(NULL);

    for (int i = 0; i < all_bombs.count; ++i) {
        bomb b = all_bombs.list[i];
        if (difftime(current_time, b.placement_time) >= BOMB_LIFETIME) {
            set_grid(b.pos.x, b.pos.y, EMPTY);

            // TODO : UGLY
            all_bombs.list[i] = all_bombs.list[--all_bombs.count];
        }
    }
}
