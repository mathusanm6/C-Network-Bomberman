#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "network_server.h"
#include "utils.h"

int main() {
    srandom(time(NULL));
    RETURN_FAILURE_IF_ERROR(init_server_network());
    RETURN_FAILURE_IF_ERROR(game_loop_server());
}
