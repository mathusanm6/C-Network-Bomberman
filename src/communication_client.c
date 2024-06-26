#include "communication_client.h"
#include "messages.h"
#include "utils.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int send_connexion_header_raw(int sock, connection_header_raw *serialized_head) {
    char *data = (char *)serialized_head;
    unsigned sent = 0;
    while (sent < sizeof(connection_header_raw)) {
        int res = send(sock, data + sent, sizeof(connection_header_raw) - sent, 0);
        RETURN_FAILURE_IF_NEG_PERROR(res, "send connection_header_raw");

        sent += res;
    }
    return EXIT_SUCCESS;
}

int send_initial_connexion_information(int sock, GAME_MODE mode) {
    initial_connection_header *head = malloc(sizeof(initial_connection_header));
    RETURN_FAILURE_IF_NULL_PERROR(head, "malloc initial_connection_header");
    head->game_mode = mode;

    connection_header_raw *serialized_head = serialize_initial_connection(head);
    free(head);

    RETURN_FAILURE_IF_NULL(serialized_head);
    int res = send_connexion_header_raw(sock, serialized_head);
    free(serialized_head);

    return res;
}

int send_ready_connexion_information(int sock, GAME_MODE mode, int id, int eq) {
    ready_connection_header *head = malloc(sizeof(ready_connection_header));
    RETURN_FAILURE_IF_NULL_PERROR(head, "malloc ready connection header");
    head->game_mode = mode;
    head->id = id;
    head->eq = eq;

    connection_header_raw *serialized_head = serialize_ready_connection(head);
    free(head);

    RETURN_FAILURE_IF_NULL(serialized_head);
    int res = send_connexion_header_raw(sock, serialized_head);
    free(serialized_head);

    return res;
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

    char *serialized_msg = client_serialize_chat_message(msg);

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

connection_information_raw *recv_connexion_information_raw(int sock) {
    connection_information_raw *head = malloc(sizeof(connection_information_raw));
    RETURN_NULL_IF_NULL_PERROR(head, "malloc connection_information_raw");
    unsigned received = 0;
    while (received < sizeof(connection_information_raw)) {
        int res = recv(sock, head + received, sizeof(connection_information_raw) - received, 0);

        if (res < 0) {
            perror("recv connection_information_raw");
            free(head);
            return NULL;
        }
        received += res;
    }
    return head;
}

connection_information *recv_connexion_information(int sock) {
    connection_information_raw *head = recv_connexion_information_raw(sock);
    RETURN_NULL_IF_NULL(head);
    connection_information *deserialized_head = deserialize_connection_information(head);
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

chat_message *recv_chat_message(int sock, uint16_t header) {
    uint8_t length;
    int res = recv_tcp(sock, &length, sizeof(uint8_t));
    RETURN_NULL_IF_NEG_PERROR(res, "recv chat_message length");

    char *message = malloc(length + 1);
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

    // Copy the header, length and message into the total_received buffer
    memcpy(total_received, &header, 2);
    memcpy(total_received + 2, &length, 1);
    memcpy(total_received + 3, message, length);

    chat_message *msg = server_deserialize_chat_message(total_received);

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

u_int16_t recv_header(int sock) {
    uint16_t header;
    int res = recv_tcp(sock, &header, sizeof(uint16_t));
    if (res < 0) {
        perror("recv header");
        return 0;
    }
    return header;
}
