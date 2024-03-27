#ifndef SRC_VIEW_H_
#define SRC_VIEW_H_

#include "./model.h"

/** Initializes graphical interface
 */
void init_view();

/** Shutdown the graphic interface
 */
void end_view();

/** Replaces the values pointed by the arguments with the terminal's
 * width and height respectively
 * Do nothing if one of the pointer is Null
 */
void get_width_height_terminal(int *, int *);

/** Updates terminal display with board data
 */
void refresh_game(board *, line *);

#endif // SRC_VIEW_H_
