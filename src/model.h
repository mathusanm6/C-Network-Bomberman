#ifndef SRC_MODEL_H_
#define SRC_MODEL_H_

#define TEXT_SIZE 100
#define PLAYER_NUM 4
#define MIN_GAMEBOARD_WIDTH 11
#define MIN_GAMEBOARD_HEIGHT 10
#define DESTRUCTIBLE_WALL_CHANCE 20
#define BOMB_LIFETIME 3 // in seconds

#include <stdbool.h>

typedef enum GAME_ACTION {
    GAME_UP = 0,
    GAME_RIGHT = 1,
    GAME_DOWN = 2,
    GAME_LEFT = 3,
    GAME_PLACE_BOMB = 4,
    GAME_NONE = 5,
    GAME_ACTIVATE_CHAT = 6,
    GAME_QUIT = 7,
    SWITCH_PLAYER = 8
} GAME_ACTION;

typedef enum CHAT_ACTION {
    CHAT_WRITE = 0,
    CHAT_ERASE = 1,
    CHAT_SEND = 2,
    CHAT_CLEAR = 3,
    CHAT_QUIT = 4,
    CHAT_GAME_QUIT = 5,
    CHAT_NONE = 6
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

typedef struct line {
    char data[TEXT_SIZE];
    int cursor;
} line;

typedef struct coord {
    int x;
    int y;
} coord;

extern line *chat_line; // line of text that can be filled in with chat

/** Initializes - The game board with the width and the height
 *              - The chat line
 *              - The current position of the player
 *              - The game mode
 */
int init_model(dimension dim, GAME_MODE mode);

/** Frees - The game board with the width and the height
 *              - The chat line
 *              - The current position of the players
 */
void free_model();

/** Frees the board
 */
void free_board(board *);

/** Returns the corresponding char of a tile
 */
char tile_to_char(TILE);

/** Returns the corresponding coordinate of an int in flatten list
 */
coord int_to_coord(int);

/** Returns the corresponding int of coordinate in flatten list
 */
int coord_to_int(int, int);

/** Returns the tile at the position (x, y) of game_board
 */
TILE get_grid(int, int);

/** Sets the tile at the position (x, y) of game_board to the last argument
 */
void set_grid(int, int, TILE);

/** Returns true if (x, y) is a coordinate outside the game_board
 */
bool is_outside_board(int x, int y);

/** Decrements the line cursor
 */
void decrement_line();

/** Nullifies the line cursor
 */
void clear_line();

/** Adds the character at the end of chat_line if it does not exceed TEXT_SIZE and increment the cursor
 */
void add_to_line(char);

/** Depending on the action, changes the player's position in the table if the argument is a move.
 */
void perform_move(GAME_ACTION, int player_id);

/**
 * Places a bomb at the current player's position, resizing the bomb array if full.
 * Updates the game grid and increments bomb count.
 */
void place_bomb(int player_id);

/** Returns a copy of the game board
 */
board *get_game_board();

/** Returns the game mode of the current game
 */
GAME_MODE get_game_mode();

bool is_player_dead(int);

/** Iterates over all bombs, removing any that have exceeded their lifetime.
 */
void update_bombs();

/** Returns true if the game is over
 */
bool is_game_over();

#endif // SRC_MODEL_H_
