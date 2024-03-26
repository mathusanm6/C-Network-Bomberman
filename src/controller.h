#ifndef SRC_CONTROLLER_H_
#define SRC_CONTROLLER_H_

typedef enum ACTION { NONE, UP, DOWN, LEFT, RIGHT, QUIT } ACTION;

#include "./model.h"

ACTION control();

int init_game();
int game();

#endif  // SRC_CONTROLLER_H_
