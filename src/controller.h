#ifndef SRC_CONTROLLER_H_
#define SRC_CONTROLLER_H_

#include "./model.h"

/** Depending on the pressed key on the keyboard, manage the player's actions and return true if the user wants to quit
 */
bool control();

/** Initialize view, controller and model to start a game
 */
int init_game();

/** Game loop
 */
int game_loop();

#endif // SRC_CONTROLLER_H_
