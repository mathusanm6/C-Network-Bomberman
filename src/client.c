#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "controller.h"
#include "network_client.h"
#include "utils.h"

#define EXIT_ASKED 2

#define NO 0
#define YES 1

typedef struct flags {
    char *mode;
    char *port;
} flags;

static GAME_MODE choosen_game_mode;

static flags *client_flags;

void free_client_flags() {
    free(client_flags);
}

int init_client_flags() {
    client_flags = malloc(sizeof(flags));
    RETURN_FAILURE_IF_NULL(client_flags);
    client_flags->mode = NULL;
    client_flags->port = NULL;

    return EXIT_SUCCESS;
}

void parse_client_flags(int argc, char *argv[]) {
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i - 1], "-p") == 0) {
            client_flags->port = argv[i];
        } else if (strcmp(argv[i - 1], "-m") == 0) {
            client_flags->mode = argv[i];
        }
    }
}

int ask_natural_number(const char *ask, unsigned minimum, unsigned maximum) {
    if (minimum > maximum) {
        fprintf(stderr, "Error with bounds.\n");
        return -1;
    }
    char buf[INT_STRING_SIZE];
    while (1) {
        memset(buf, 0, sizeof(buf));

        printf("%s (EXIT to leave the game):\n", ask);
        fgets(buf, INT_STRING_SIZE - 1, stdin);

        size_t len_buf = strlen(buf);
        if (len_buf > 0 && buf[len_buf - 1] == '\n') {
            buf[len_buf - 1] = '\0';
        }

        if (strcmp("EXIT", buf) == 0) {
            printf("You have left the information input.\n");
            return -1;
        }

        int n = parse_unsigned_within_bounds(buf, minimum, maximum);
        if (n < 0) {
            printf("It's not a correct number or it exceeds limits (min: %u, max: %u).\n", minimum, maximum);
            continue;
        }
        return n;
    }
}

int try_to_init_mode_client() {
    int mode;
    if (client_flags->mode == NULL) {
        mode = ask_natural_number("Do you want to play in team ? 0 for no and 1 yes", NO, YES);
        if (mode < 0) {
            return EXIT_ASKED;
        }
    } else {
        mode = parse_unsigned_within_bounds(client_flags->mode, NO, YES);
        if (mode < 0) {
            printf("Your mode argument is not valid, it has to be between %d and %d.\n", NO, YES);
            return EXIT_FAILURE;
        }
    }

    if (mode == 0) {
        choosen_game_mode = SOLO;
    } else {
        choosen_game_mode = TEAM;
    }
    return EXIT_SUCCESS;
}

int try_to_init_port_and_connect_client() {
    RETURN_FAILURE_IF_ERROR(init_tcp_socket());

    while (1) {
        int r;
        if (client_flags->port == NULL) {
            r = ask_natural_number("Give the port of the server you want to connect to", MIN_PORT, MAX_PORT);

            if (r < 0) {
                return EXIT_ASKED;
            }
        } else {
            r = parse_unsigned_within_bounds(client_flags->port, MIN_PORT, MAX_PORT);
            if (r < 0) {
                printf("Your port argument is not valid, it has to be between %d and %d.\n", MIN_PORT, MAX_PORT);
                return EXIT_FAILURE;
            }
        }
        set_tcp_port(r);

        if (try_to_connect_tcp() < 0) {
            int rep = ask_natural_number(
                "You couldn't connect to the server, would you like to try another port ? 1 for yes 0 for no", NO, YES);
            if (rep <= 0) {
                return EXIT_ASKED;
            }
        } else {
            return EXIT_SUCCESS;
        }
    }
}

int try_to_init_client() {
    int r = try_to_init_mode_client();

    if (r != EXIT_SUCCESS) {
        return r;
    }

    return try_to_init_port_and_connect_client();
}

int be_ready() {
    int res = ask_natural_number("Type 0 if you are ready", 0, 0);
    if (res != EXIT_SUCCESS) {
        return EXIT_ASKED;
    }
    printf("You are ready, wait for other players.\n");
    return send_ready_to_play(choosen_game_mode);
}

int main(int argc, char *argv[]) {
    RETURN_FAILURE_IF_ERROR(init_client_flags());
    parse_client_flags(argc, argv);
    int r = try_to_init_client();
    free_client_flags();

    if (r != EXIT_SUCCESS) {
        return r;
    }
    connection_information *info = start_initialisation_game(choosen_game_mode);
    RETURN_FAILURE_IF_NULL(info)
    printf("You are player %d.\n", info->id + 1);

    r = be_ready();
    if (r != EXIT_SUCCESS) {
        goto error;
    }

    if (init_game(info->id, info->eq, choosen_game_mode) == EXIT_FAILURE) {

        goto error;
    }

    r = game_loop();

    goto error;

error:
    free(info);
    free_internal_info();
    return r;
}
