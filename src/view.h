#ifndef SRC_VIEW_H_
#define SRC_VIEW_H_

#include "./model.h"

#include <ncurses.h>

#define MIN_GAME_WIDTH 80
#define MIN_GAME_HEIGHT 24
#define PADDING_SCREEN_TOP 1
#define PADDING_SCREEN_LEFT 2
#define PADDING_PLAYABLE_TOP 2
#define PADDING_PLAYABLE_LEFT 4

/** Initializes graphical user interface
 */
void init_view();

/** Shutdown the graphic user interface
 */
void end_view();

/**
 * Replaces the values pointed by the arguments with the computed board dimension
 * Does nothing if the pointer is NULL
 */
void get_computed_board_dimension(dimension *);

/** Updates terminal display with board data
 */
void refresh_game(board *, line *);

#endif // SRC_VIEW_H_
