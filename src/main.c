#include <stdlib.h>

#include "controller.h"

int main() {
    if (init_game() < 0) {
        return EXIT_FAILURE;
    }
    return game();
}
