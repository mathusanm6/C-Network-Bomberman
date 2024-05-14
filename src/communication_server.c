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
    int a;
    if ((a = sendto(sock, message, message_length, 0, (struct sockaddr *)addr_mult, sizeof(struct sockaddr_in6))) < 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int send_game_board(int sock, struct sockaddr_in6 *addr_mult, uint16_t num, board *board_) {
    game_board_information *head = malloc(sizeof(game_board_information));
    RETURN_FAILURE_IF_NULL(head);

    head->num = num;
    head->width = board_->dim.width;
    head->height = board_->dim.height;
    head->board = malloc(head->width * head->height * sizeof(TILE));

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

int send_tcp(int sock, const void *buffer, uint8_t size) {
    unsigned sent = 0;
    while (sent < size) {
        int res = send(sock, buffer + sent, size - sent, 0);
        if (res < 0) {
            perror("send tcp");
            return EXIT_FAILURE;
        }
        if (res == 0) {
            // TODO: handle connection closed
            perror("send tcp: connection closed");
            return EXIT_FAILURE;
        }
        sent += res;
    }
    return EXIT_SUCCESS;
}

int send_chat_message(int sock, chat_message_type type, int id, int eq, uint8_t message_length, char *message) {
    chat_message *msg = malloc(sizeof(chat_message));
    RETURN_FAILURE_IF_NULL_PERROR(msg, "malloc chat_message");
    msg->type = type;
    msg->id = id;
    msg->eq = eq;
    msg->message_length = message_length;
    msg->message = malloc(message_length + 1);
    if (msg->message == NULL) {
        free(msg);
        perror("malloc chat_message message");
        return EXIT_FAILURE;
    }
    strncpy(msg->message, message, message_length);
    msg->message[message_length] = '\0'; // Ensure the message is null-terminated

    char *serialized_msg = server_serialize_chat_message(msg);
    free(msg->message);
    free(msg);
    RETURN_FAILURE_IF_NULL(serialized_msg);

    int res = send_tcp(sock, serialized_msg, sizeof(chat_message) + message_length);
    free(serialized_msg);
    return res;
}

connection_header_raw *recv_connexion_header_raw(int sock) {
    connection_header_raw *head = malloc(sizeof(connection_header_raw));
    RETURN_NULL_IF_NULL_PERROR(head, "malloc connection_header_raw");
    unsigned received = 0;
    while (received < sizeof(connection_header_raw)) {
        int res = recv(sock, head + received, sizeof(connection_header_raw) - received, 0);
        if (res < 0) {
            perror("recv connection_header_raw");
            free(head);
            return NULL;
        }
        received += res;
    }
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
    connection_header_raw *head = recv_connexion_header_raw(sock);
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

int recv_tcp(int sock, void *buffer, int size) {
    int received = 0;
    while (received < size) {
        int res = recv(sock, buffer + received, size - received, 0);
        if (res < 0) {
            perror("recv tcp");
            return EXIT_FAILURE;
        }
        if (res == 0) {
            // TODO: handle connection closed
            perror("recv tcp: connection closed");
            return EXIT_FAILURE;
        }
        received += res;
    }
    return EXIT_SUCCESS;
}

chat_message *recv_chat_message(int sock) {
    uint16_t header;
    int res = recv_tcp(sock, &header, sizeof(uint16_t));
    RETURN_NULL_IF_NEG_PERROR(res, "recv chat_message header");

    uint8_t length;
    res = recv_tcp(sock, &length, sizeof(uint8_t));
    RETURN_NULL_IF_NEG_PERROR(res, "recv chat_message length");

    char *message = malloc(length + 1);
    RETURN_NULL_IF_NULL_PERROR(message, "malloc chat_message message");
    res = recv_tcp(sock, message, length);
    if (res < 0) {
        perror("recv chat_message message");
        free(message);
        return NULL;
    }
    message[length] = '\0'; // Ensure the message is null-terminated

    char *total_received = malloc(sizeof(uint16_t) + sizeof(uint8_t) + length);
    if (total_received == NULL) {
        perror("malloc chat_message total_received");
        free(message);
        return NULL;
    }
    memcpy(total_received, &header, sizeof(uint16_t));
    memcpy(total_received + sizeof(uint16_t), &length, sizeof(uint8_t));
    memcpy(total_received + sizeof(uint16_t) + sizeof(uint8_t), message, length);

    chat_message *msg = server_deserialize_chat_message(total_received);
    free(total_received);
    free(message);

    return msg;
}