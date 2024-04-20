#ifndef SRC_MODEL_H_
#define SRC_MODEL_H_

#define TEXT_SIZE 100
#define PLAYER_NUM 4
#define MIN_GAMEBOARD_WIDTH 11
#define MIN_GAMEBOARD_HEIGHT 10
#define MAX_CHAT_HISTORY_LEN 10
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
    GAME_SWITCH_PLAYER = 8
} GAME_ACTION;

typedef enum CHAT_ACTION {
    CHAT_WRITE = 0,
    CHAT_ERASE = 1,
    CHAT_SEND = 2,
    CHAT_TOGGLE_WHISPER = 3,
    CHAT_CLEAR = 4,
    CHAT_QUIT = 5,
    CHAT_GAME_QUIT = 6,
    CHAT_SWITCH_PLAYER = 7,
    CHAT_NONE = 8
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

typedef struct chat_node {
    int sender;
    char message[TEXT_SIZE];
    bool whispered;
    struct chat_node *next;
} chat_node;

typedef struct chat_history {
    chat_node *head;
    int count;
} chat_history;

typedef struct chat_line {
    char data[TEXT_SIZE];
    int cursor;
} chat_line;

typedef struct chat {
    chat_history *history;
    chat_line *line;
    bool on_focus;
    bool whispering;
} chat;

extern chat *chat_;

/** Initializes - The game board with the width and the height
 *              - The chat line
 *              - The current position of the player
 *              - The game mode
 */
int init_model(dimension dim, GAME_MODE mode, unsigned int game_id);

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

/** Decrements the line cursor
 */
void decrement_line();

/** Nullifies the line cursor
 */
void clear_line();

/** Adds the character at the end of chat_line if it does not exceed TEXT_SIZE and increment the cursor
 */
void add_to_line(char);

// TODO! : ADD COMMENTS
void add_message(int sender, char *msg, bool whispered);

void toggle_whispering();

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

/** Returns true if the game is over
 */
bool is_game_over(unsigned int game_id);

#endif // SRC_MODEL_H_
