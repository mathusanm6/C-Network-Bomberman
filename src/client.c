#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "communication.h"
#include "messages.h"
#include "model.h"
#include "utils.h"

#define EXIT_ASKED 2

#define NO 0
#define YES 1

#define MIN_PORT 1024
#define MAX_PORT 49151

typedef struct flags {
    char *mode;
    char *port;
} flags;

static GAME_MODE choosen_game_mode;
static int id;
static int eq;

static flags *client_flags;
static const char *IP_SERVER = "::1";

static int sock_tcp = -1;

static uint16_t port_tcp = 0;
static uint16_t port_udp = 0;
static uint16_t port_mult = 0;

static uint16_t adrmdiff[8];

void close_socket(int sock) {
    if (sock != 0) {
        close(sock);
    }
}

void free_client_flags() {
    free(client_flags);
}

int init_socket(int *sock, bool is_tcp) {
    if (is_tcp) {
        *sock = socket(PF_INET6, SOCK_STREAM, 0);
    } else {
        *sock = socket(PF_INET6, SOCK_DGRAM, 0);
    }
    if (*sock < 0) {
        perror("socket creation");
        *sock = -1;
        return EXIT_FAILURE;
    }
    int no = 0;
    if (setsockopt(*sock, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no)) < 0) {
        perror("setsockopt polymorphism");
        close_socket(*sock);
        *sock = -1;
        return EXIT_FAILURE;
    }
    int ok = 1;
    if (setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(ok)) < 0) {
        perror("setsockopt reuseaddr");
        close(*sock);
        *sock = -1;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int init_client_flags() {
    client_flags = malloc(sizeof(flags));
    if (client_flags == NULL) {
        return EXIT_FAILURE;
    }
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

struct sockaddr_in6 *prepare_address() {
    struct sockaddr_in6 *addrsock = malloc(sizeof(struct sockaddr_in6));
    if (addrsock == NULL) {
        perror("malloc addrsock");
        return NULL;
    }
    memset(addrsock, 0, sizeof(struct sockaddr_in6));
    addrsock->sin6_family = AF_INET6;
    addrsock->sin6_port = port_tcp;
    inet_pton(AF_INET6, IP_SERVER, &addrsock->sin6_addr);
    return addrsock;
}

int try_to_connect(struct sockaddr_in6 *addr) {
    return connect(sock_tcp, (struct sockaddr *)addr, sizeof(struct sockaddr_in6));
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

        int n = give_natural_number_of_string_between(buf, minimum, maximum);
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
        mode = give_natural_number_of_string_between(client_flags->mode, NO, YES);
        if (mode < 0) {
            printf("Your mode argument is no valid, it has to be between %d and %d.\n", NO, YES);
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
    if (init_socket(&sock_tcp, true) < 0) {
        return EXIT_FAILURE;
    }

    while (1) {
        int r;
        if (client_flags->port == NULL) {
            r = ask_natural_number("Give the port of the server you want to connect to", MIN_PORT, MAX_PORT);

            if (r < 0) {
                return EXIT_ASKED;
            }
        } else {
            r = give_natural_number_of_string_between(client_flags->port, MIN_PORT, MAX_PORT);
            if (r < 0) {
                printf("Your port argument is no valid, it has to be between %d and %d.\n", MIN_PORT, MAX_PORT);
                return EXIT_FAILURE;
            }
        }
        port_tcp = htons(r);

        struct sockaddr_in6 *addrsock = prepare_address();

        if (try_to_connect(addrsock) < 0) {
            port_tcp = 0;
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

int start_initialisation_game() {
    if (send_initial_connexion_information(sock_tcp, choosen_game_mode) < 0) {
        return EXIT_FAILURE;
    }
    printf("You have to wait other players.\n");
    connection_information *head = recv_connexion_information(sock_tcp);
    printf("test\n");
    if (head == NULL) {
        return EXIT_FAILURE;
    }
    port_udp = head->portudp;
    port_mult = head->portmdiff;
    id = head->id;
    eq = head->eq;
    for (unsigned i = 0; i < 8; i++) {
        adrmdiff[i] = head->adrmdiff[i];
    }
    printf("The server is ready.\n");
    int res = ask_natural_number("Type 0 if you are ready", 0, 0);
    if (res != 0) {
        return EXIT_ASKED;
    }
    send_ready_connexion_information(sock_tcp, choosen_game_mode, id, eq);
    printf("It's ok !");
    pause();
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    if (init_client_flags() < 0) {
        return EXIT_FAILURE;
    }
    parse_client_flags(argc, argv);
    int r = try_to_init_client();
    free_client_flags();

    if (r != EXIT_SUCCESS) {
        return r;
    }
    printf("%d\n", ntohs(port));
    close_tcp_socket();
    /*RETURN_FAILURE_IF_ERROR(init_game());

    return game_loop();*/
    return EXIT_FAILURE;
}
