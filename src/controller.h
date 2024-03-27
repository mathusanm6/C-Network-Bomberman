#ifndef SRC_CONTROLLER_H_
#define SRC_CONTROLLER_H_

#include "./model.h"

/** Depending on the pressed key on the keyboard, returns the action, or adds the character to chat_line
 */
ACTION control();

/** Initialize view, controller and model to start a game
 */
int init_game();

/** Game loop
 */
int game_loop();

#endif // SRC_CONTROLLER_H_
