#ifndef SRC_CONTROLLER_H_
#define SRC_CONTROLLER_H_

#include "./model.h"
#include <stdbool.h>

/** Initialize view, controller and model to start a game
 */
int init_game(int id, int eq, GAME_MODE);

/** Game loop
 */
int game_loop();

#endif // SRC_CONTROLLER_H_
