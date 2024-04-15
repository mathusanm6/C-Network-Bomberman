#include "controller.h"
#include "utils.h"

int main() {
    RETURN_FAILURE_IF_ERROR(init_game());

    return game_loop();
}
