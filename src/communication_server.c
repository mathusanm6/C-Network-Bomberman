#include "communication_server.h"
#include "messages.h"
#include "model.h"
#include "utils.h"

#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

int send_connexion_information_raw(int sock, connection_information_raw *serialized_head) {
    char *data = (char *)serialized_head;
    unsigned sent = 0;
    while (sent < sizeof(connection_information_raw)) {
        int res = send(sock, data + sent, sizeof(connection_information_raw) - sent, 0);

        if (res < 0) {
            perror("send connection_information");
            return EXIT_FAILURE;
        }
        sent += res;
    }
    return EXIT_SUCCESS;
}

int send_connexion_information(int sock, GAME_MODE mode, int id, int eq, int portudp, int portmdiff,
                               uint16_t adrmdiff[8]) {
    connection_information *head = malloc(sizeof(connection_information));
    RETURN_FAILURE_IF_NULL_PERROR(head, "malloc connection_information");
    head->game_mode = mode;
    head->id = id;
    head->eq = eq;
    head->portudp = portudp;
    head->portmdiff = portmdiff;

    for (unsigned i = 0; i < 8; i++) {
        head->adrmdiff[i] = adrmdiff[i];
    }
    connection_information_raw *serialized_head = serialize_connection_information(head);
    free(head);

    RETURN_FAILURE_IF_NULL(serialized_head);
    int res = send_connexion_information_raw(sock, serialized_head);
    free(serialized_head);

    return res;
}
int send_string_to_clients_multicast(int sock, struct sockaddr_in6 *addr_mult, char *message, size_t message_length) {
    printf("Sending message to multicast\n");
    printf("Message : %s\n", message);
    printf("Message length : %ld\n", message_length);
    int a;
    if ((a = sendto(sock, message, message_length, 0, (struct sockaddr *)addr_mult, sizeof(struct sockaddr_in6))) < 0) {
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < message_length; i++) {
        printf("%d ", message[i]);
    }
    printf("Message sent to multicast, %d\n", a);
    return EXIT_SUCCESS;
}

int send_game_board(int sock, struct sockaddr_in6 *addr_mult, uint16_t num, board *board_) {
    game_board_information *head = malloc(sizeof(game_board_information));
    RETURN_FAILURE_IF_NULL(head);

    head->num = num;
    head->width = board_->dim.width;
    head->height = board_->dim.height;
    head->board = malloc(head->width * head->height * sizeof(TILE));

    printf("Sending board\n");
    printf("Num : %d\n", head->num);
    printf("Width : %d\n", head->width);
    printf("Height : %d\n", head->height);

    for (unsigned i = 0; i < head->width * head->height; i++) {
        head->board[i] = board_->grid[i];
    }

    char *serialized_head = serialize_game_board(head);
    free(head->board);
    free(head);
    RETURN_FAILURE_IF_NULL(serialized_head);

    size_t len_serialized_head = 6 + board_->dim.width * board_->dim.height;

    return send_string_to_clients_multicast(sock, addr_mult, serialized_head, len_serialized_head);
}

int send_game_update(int sock, struct sockaddr_in6 *addr_mult, int num, tile_diff *diff, uint8_t nb) {
    game_board_update *head = malloc(sizeof(game_board_update));
    RETURN_FAILURE_IF_NULL(head);

    head->num = num;
    head->diff = diff;
    head->nb = nb;

    char *serialized_head = serialize_game_board_update(head);
    free(head);
    RETURN_FAILURE_IF_NULL(serialized_head);
    size_t len_serialized_head = 5 + nb * 3;

    return send_string_to_clients_multicast(sock, addr_mult, serialized_head, len_serialized_head);
}

connection_header_raw *recv_connexion_header_raw(int sock) {
    printf("testdebraw %d\n", sock);
    connection_header_raw *head = malloc(sizeof(connection_header_raw));
    printf("testaftermalloc %d\n", sock);
    RETURN_NULL_IF_NULL_PERROR(head, "malloc connection_header_raw");
    unsigned received = 0;
    while (received < sizeof(connection_header_raw)) {
        int res = recv(sock, head + received, sizeof(connection_header_raw) - received, 0);
        printf("testafterrecv %d\n", sock);

        if (res < 0) {
            perror("recv connection_header_raw");
            free(head);
            return NULL;
        }
        received += res;
    }
    printf("testfinraw\n");
    return head;
}

initial_connection_header *recv_initial_connection_header(int sock) {
    connection_header_raw *head = recv_connexion_header_raw(sock);
    RETURN_NULL_IF_NULL(head);
    initial_connection_header *deserialized_head = deserialize_initial_connection(head);
    free(head);
    return deserialized_head;
}

ready_connection_header *recv_ready_connexion_header(int sock) {
    printf("testdebready %d\n", sock);
    connection_header_raw *head = recv_connexion_header_raw(sock);
    printf("testfinready %d\n", sock);
    RETURN_NULL_IF_NULL(head);
    ready_connection_header *deserialized_head = deserialize_ready_connection(head);
    free(head);
    return deserialized_head;
}

char *recv_string_udp(int sock, size_t size) {
    char *res = malloc(size);
    RETURN_NULL_IF_NULL(res);
    if (recvfrom(sock, res, size, 0, NULL, 0) < 0) {
        perror("recvfrom string udp");
        free(res);
        return NULL;
    }
    return res;
}

game_action *recv_game_action(int sock) {
    char *head = recv_string_udp(sock, sizeof(game_action));
    RETURN_NULL_IF_NULL(head);

    game_action *deserialized_head = deserialize_game_action(head);
    free(head);
    return deserialized_head;
}
