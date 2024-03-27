#ifndef SRC_MODEL_H_
#define SRC_MODEL_H_

#define TEXT_SIZE 255

#include <stdbool.h>

typedef enum ACTION { NONE, UP, DOWN, LEFT, RIGHT, QUIT } ACTION;

typedef struct board {
    char *grid;
    int width;
    int height;
} board;

typedef struct line {
    char data[TEXT_SIZE];
    int cursor;
} line;

typedef struct pos {
    int x;
    int y;
} pos;

extern board *game_board; // playing surface
extern line *chat_line;   // line of text that can be filled in with chat
extern pos *current_pos;  // current position of the player

/** Initializes - The game board with the width and the height
 *              - The chat line
 *              - The current position of the player
 */
int init_model(int, int);

/** Frees - The game board with the width and the height
 *              - The chat line
 *              - The current position of the player
 */
void free_model();

/** Returns the character at the position (x, y) of game_board
 */
int get_grid(int, int);

/** Sets the character to the last argument at the position (x, y) of game_board
 */
void set_grid(int, int, int);

/** Decrements the line cursor
 */
void decrement_line();

/** Adds the character at the end of chat_line if it does not exceed TEXT_SIZE and increment the cursor
 */
void add_to_line(char);

/** Depending on the action, changes the player's position in the table if a move has been made.
 * Returns true if the player has made the quit action.
 */
bool perform_action(ACTION);

#endif // SRC_MODEL_H_
