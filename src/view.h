#ifndef SRC_VIEW_H_
#define SRC_VIEW_H_

#include "./model.h"

#include <ncurses.h>

/** Initializes graphical user interface
 */
int init_view();

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
void refresh_game(board *, chat *);

#endif // SRC_VIEW_H_
