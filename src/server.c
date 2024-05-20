#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <time.h>
#include <unistd.h>

#include "network_server.h"
#include "utils.h"

typedef struct flags {
    char *connexion_port;
} flags;

static flags *server_flags;

int init_server_flags() {
    server_flags = malloc(sizeof(flags));
    RETURN_FAILURE_IF_NULL_PERROR(server_flags, "malloc server_flags");
    server_flags->connexion_port = NULL;

    return EXIT_SUCCESS;
}

void parse_client_flags(int argc, char *argv[]) {
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i - 1], "-p") == 0) {
            server_flags->connexion_port = argv[i];
        }
    }
}

int main(int argc, char *argv[]) {
    srandom(time(NULL));
    RETURN_FAILURE_IF_ERROR(init_server_flags());
    parse_client_flags(argc, argv);
    uint16_t connexion_port = 0;
    if (server_flags->connexion_port != NULL) {
        int r = parse_unsigned_within_bounds(server_flags->connexion_port, MIN_PORT, MAX_PORT);
        if (r >= 0) {
            connexion_port = r;
        } else {
            fprintf(stderr, "The port is not valid.\n");
            free(server_flags);
            return EXIT_FAILURE;
        }
    }
    free(server_flags);

    RETURN_FAILURE_IF_ERROR(init_socket_tcp());

    init_state(connexion_port);
    RETURN_FAILURE_IF_ERROR(game_loop_server());
}
