#ifndef SRC_CONTROLLER_H_
#define SRC_CONTROLLER_H_

typedef enum ACTION { NONE, UP, DOWN, LEFT, RIGHT, QUIT } ACTION;

#include "./model.h"

/** Depending on the press key on the keyboard, returns the action, or adds the character to chat_line
 */
ACTION control();

/** Initialize view, controller and model to start a game
 */
int init_game();

/** Game loop
 */
int game();

#endif // SRC_CONTROLLER_H_
