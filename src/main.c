#include <stdlib.h>

#include "controller.h"

int main() {
    if (init_game() == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }
    return game_loop();
}
