#ifndef SRC_MODEL_H_
#define SRC_MODEL_H_

#include <stdbool.h>

#define TEXT_SIZE 255
#define PLAYER_NUM 4
#define MIN_GAMEBOARD_WIDTH 11
#define MIN_GAMEBOARD_HEIGHT 10
#define DESTRUCTIBLE_WALL_CHANCE 20

typedef enum ACTION {
    UP = 0,
    RIGHT = 1,
    DOWN = 2,
    LEFT = 3,
    PLACE_BOMB = 4,
    NONE = 5,
    CHAT_WRITE = 6,
    CHAT_ERASE = 7,
    QUIT = 8,
    SWITCH_PLAYER = 9
} ACTION;

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

/** Adds the character at the end of chat_line if it does not exceed TEXT_SIZE and increment the cursor
 */
void add_to_line(char);

/** Depending on the action, changes the player's position in the table if the argument is a move.
 */
void perform_move(ACTION, int player_id);

/** Returns a copy of the game board
 */
board *get_game_board();

/** Returns the game mode of the current game
 */
GAME_MODE get_game_mode();

#endif // SRC_MODEL_H_
