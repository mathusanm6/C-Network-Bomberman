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
        int res = send(sock, (char *)buffer + sent, size - sent, 0);
        if (res < 0) {
            perror("send tcp");
            return EXIT_FAILURE;
        }
        if (res == 0) {
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
    msg->message = malloc(message_length);
    if (msg->message == NULL) {
        free(msg);
        perror("malloc chat_message message");
        return EXIT_FAILURE;
    }
    strncpy(msg->message, message, message_length);

    char *serialized_msg = server_serialize_chat_message(msg);
    if (serialized_msg == NULL) {
        free(msg->message);
        free(msg);
        return EXIT_FAILURE;
    }

    int res = send_tcp(sock, serialized_msg, 3 + message_length);
    free(msg->message);
    free(msg);
    free(serialized_msg);
    return res;
}

int send_game_over(int sock, GAME_MODE mode, int id, int eq) {
    game_end *head = malloc(sizeof(game_end));
    RETURN_FAILURE_IF_NULL_PERROR(head, "malloc game_end");
    head->game_mode = mode;
    head->id = id;
    head->eq = eq;

    char *serialized_head = serialize_game_end(head);
    free(head);
    RETURN_FAILURE_IF_NULL(serialized_head);

    int res = send_tcp(sock, serialized_head, 2);
    free(serialized_head);

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
        int res = recv(sock, (char *)buffer + received, size - received, 0);
        if (res < 0) {
            perror("recv tcp");
            return EXIT_FAILURE;
        }
        if (res == 0) {
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

    char *message = malloc(length);
    RETURN_NULL_IF_NULL_PERROR(message, "malloc chat_message message");
    res = recv_tcp(sock, message, length * sizeof(char));
    if (res < 0) {
        perror("recv chat_message message");
        free(message);
        return NULL;
    }

    // Allocate memory for the total received message
    char *total_received = malloc(3 + length);
    if (total_received == NULL) {
        perror("malloc chat_message total_received");
        free(message);
        return NULL;
    }

    // Copy the header, length and message into the total received message
    memcpy(total_received, &header, 2);
    memcpy(total_received + 2, &length, 1);
    memcpy(total_received + 3, message, length);

    // Deserialize the chat message
    chat_message *msg = client_deserialize_chat_message(total_received);
    if (msg == NULL) {
        perror("deserialize chat_message");
        free(total_received);
        free(message);
        return NULL;
    }

    free(total_received);
    free(message);

    return msg;
}