#ifndef SRC_MODEL_H_
#define SRC_MODEL_H_

#include "chat_model.h"
#include "constants.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum GAME_ACTION {
    GAME_UP = 0,
    GAME_RIGHT = 1,
    GAME_DOWN = 2,
    GAME_LEFT = 3,
    GAME_PLACE_BOMB = 4,
    GAME_NONE = 5,
    GAME_CHAT_MODE_START = 6,
    GAME_QUIT = 7,
} GAME_ACTION;

typedef enum CHAT_ACTION {
    CHAT_WRITE = 0,
    CHAT_ERASE = 1,
    CHAT_SEND = 2,
    CHAT_TOGGLE_WHISPER = 3,
    CHAT_CLEAR = 4,
    CHAT_MODE_QUIT = 5,
    CHAT_GAME_QUIT = 6,
    CHAT_NONE = 7
} CHAT_ACTION;

typedef enum TILE {
    EMPTY = 0,
    INDESTRUCTIBLE_WALL = 1,
    DESTRUCTIBLE_WALL = 2,
    BOMB = 3,
    EXPLOSION = 4,
    PLAYER_1 = 5,
    PLAYER_2 = 6,
    PLAYER_3 = 7,
    PLAYER_4 = 8,
    VERTICAL_BORDER = 9,
    HORIZONTAL_BORDER = 10
} TILE;

typedef enum GAME_MODE { TEAM, SOLO } GAME_MODE;

typedef struct dimension {
    int width;
    int height;
} dimension;

typedef struct board {
    char *grid;
    dimension dim;
} board;

typedef struct coord {
    int x;
    int y;
} coord;

typedef struct player_action {
    int id;
    GAME_ACTION action;
} player_action;

typedef struct tile_diff {
    uint8_t x;
    uint8_t y;
    TILE tile;
} tile_diff;

/** Initializes - The game board with the width and the height
 *              - The chat line
 *              - The current position of the player
 *              - The game mode
 *  It returns the id of the game
 */
int init_model(dimension dim, GAME_MODE mode);

/** Removes all the games
 */
void reset_games();

/** Frees - The game board with the width and the height
 *              - The chat line
 *              - The current position of the players
 */
void free_model(unsigned int game_id);

/** Frees the board
 */
void free_board(board *);

/** Returns the corresponding char of a tile
 */
char tile_to_char(TILE);

/** Returns the corresponding coordinate of an int in flatten list
 */
coord int_to_coord(int, unsigned int game_id);

/** Returns the corresponding int of coordinate in flatten list
 */
int coord_to_int_dim(int, int, dimension);

/** Returns the corresponding int of coordinate in flatten list
 */
int coord_to_int(int, int, unsigned int game_id);

/** Returns the tile at the position (x, y) of game_board
 */
TILE get_grid(int, int, unsigned int game_id);

/** Sets the tile at the position (x, y) of game_board to the last argument
 */
void set_grid(int, int, TILE, unsigned int game_id);

/** Returns true if (x, y) is a coordinate outside the game_board
 */
bool is_outside_board(int x, int y, unsigned int game_id);

bool is_move(GAME_ACTION);

/** Depending on the action, changes the player's position in the table if the argument is a move.
 */
void perform_move(GAME_ACTION, int player_id, unsigned int game_id);

/**
 * Places a bomb at the current player's position, resizing the bomb array if full.
 * Updates the game grid and increments bomb count.
 */
void place_bomb(int player_id, unsigned int game_id);

/** Returns a copy of the game board
 */
board *get_game_board(unsigned int game_id);

/** Returns the game mode of the current game
 */
GAME_MODE get_game_mode(unsigned int game_id);

bool is_player_dead(int, unsigned int game_id);

/** Iterates over all bombs, removing any that have exceeded their lifetime.
 */
void update_bombs(unsigned int game_id);

/** Returns the tiles of the board of game_id after actions modication, which differates with current board, and change
 * size_tile_diff with the size of the result
 */
tile_diff *update_game_board(unsigned game_id, player_action *actions, size_t nb_game_actions,
                             unsigned *size_tile_diff);

/** Returns true if the game is over
 */
bool is_game_over(unsigned int game_id);

/** Returns the winner player of the game if solo mode, -1 otherwise
 */
int get_winner_solo(unsigned int game_id);

/** Returns the winner team of the game if team mode, -1 otherwise
 */
int get_winner_team(unsigned int game_id);

/** Returns the chat information from the game
 */
chat *get_chat(unsigned int game_id);

#endif // SRC_MODEL_H_
