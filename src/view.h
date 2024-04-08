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

typedef struct window {
    dimension dim;
    int start_y;
    int start_x;
    WINDOW *win;
} window;

typedef struct padding {
    int top;
    int left;
} padding; // Padding right and bottom are not needed (because of inferring)

/** Initializes graphical user interface
 */
void init_view();

/** Shutdown the graphic user interface
 */
void end_view();

/** Replaces the values pointed by the arguments with the terminal's
 * height and width respectively
 * Do nothing if one of the pointer is Null
 */
void get_height_width_terminal(dimension *);

/** Replaces the values pointed by the arguments with the height and width
 * of the playable area respectively while taking into account the screen's size
 * Does nothing if one of the pointer is NULL
 */
void get_height_width_playable(dimension *, dimension);

/** Adds padding to the dimension
 */
void add_padding(dimension *, padding);

/** Updates terminal display with board data
 */
void refresh_game(board *, line *);

#endif // SRC_VIEW_H_
