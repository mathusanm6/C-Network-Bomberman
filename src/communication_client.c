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

        if (res < 0) {
            perror("send connection_header_information");
            return EXIT_FAILURE;
        }
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

message_header *recv_header_multidiff(const udp_information *info) {
    uint16_t header;
    int res = recvfrom(info->sock, &header, sizeof(uint16_t), 0, (struct sockaddr *)info->addr, info->addr_len);
    if (res < 0) {
        perror("recvfrom header");
        return NULL;
    }

    return deserialize_message_header(header);
}

void recvfrom_full(const udp_information *info, char *buffer, int size) {
    int received = 0;
    while (received < size) {
        int res =
            recvfrom(info->sock, buffer + received, size - received, 0, (struct sockaddr *)info->addr, info->addr_len);
        if (res < 0) {
            perror("recvfrom message");
            return;
        }
        received += res;
    }
}

char *recv_game_board_information(const udp_information *info, message_header *header) {
    uint16_t message_num;
    int res = recvfrom(info->sock, &message_num, sizeof(uint16_t), 0, (struct sockaddr *)info->addr, info->addr_len);
    if (res < 0) {
        perror("recvfrom message_num");
        return NULL;
    }

    uint8_t height;
    res = recvfrom(info->sock, &height, sizeof(uint8_t), 0, (struct sockaddr *)info->addr, info->addr_len);
    if (res < 0) {
        perror("recvfrom height");
        return NULL;
    }

    uint8_t width;
    res = recvfrom(info->sock, &width, sizeof(uint8_t), 0, (struct sockaddr *)info->addr, info->addr_len);
    if (res < 0) {
        perror("recvfrom width");
        return NULL;
    }

    int message_size = height * width;
    int non_board_size = 2 + 2 + 1 + 1; // 2 for the header, 2 for the message_num, 1 for the height, 1 for the width

    char *message = malloc(message_size + non_board_size);
    RETURN_NULL_IF_NULL_PERROR(message, "malloc message");

    uint16_t header_serialized = serialize_message_header(header);
    memcpy(message, &header_serialized, 2);
    memcpy(message + 2, &message_num, 2);
    memcpy(message + 4, &height, 1);
    memcpy(message + 5, &width, 1);

    recvfrom_full(info, message + non_board_size, message_size);

    return message;
}

char *recv_game_update(const udp_information *info, message_header *header) {
    uint16_t message_num;
    int res = recvfrom(info->sock, &message_num, sizeof(uint16_t), 0, (struct sockaddr *)info->addr, info->addr_len);
    if (res < 0) {
        perror("recvfrom message_num");
        return NULL;
    }

    uint8_t updated_tiles;
    res = recvfrom(info->sock, &updated_tiles, sizeof(uint8_t), 0, (struct sockaddr *)info->addr, info->addr_len);
    if (res < 0) {
        perror("recvfrom height");
        return NULL;
    }

    int message_size = updated_tiles * 3;
    int non_update_size = 2 + 2 + 1; // 2 for the header, 2 for the message_num, 1 for the number of updated tiles

    char *message = malloc(message_size + non_update_size);
    RETURN_NULL_IF_NULL_PERROR(message, "malloc message");

    uint16_t header_serialized = serialize_message_header(header);
    memcpy(message, &header_serialized, 2);
    memcpy(message + 2, &message_num, 2);
    memcpy(message + 4, &updated_tiles, 1);

    recvfrom_full(info, message + non_update_size, message_size);

    return message;
}

recieved_game_message *recv_game_message(udp_information *info) {
    message_header *header = recv_header_multidiff(info);
    RETURN_NULL_IF_NULL(header);

    char *message = NULL;
    game_message_type type;

    switch (header->codereq) {
        case 11:
            message = recv_game_board_information(info, header);
            free(header);
            RETURN_NULL_IF_NULL(message);

            type = GAME_BOARD_INFORMATION;
            break;
        case 12:
            message = recv_game_update(info, header);
            free(header);
            RETURN_NULL_IF_NULL(message);

            type = GAME_BOARD_UPDATE;
            break;
        default:
            free(header);
            return NULL;
    }

    recieved_game_message *recieved = malloc(sizeof(recieved_game_message));
    RETURN_NULL_IF_NULL_PERROR(recieved, "malloc recieved_game_message");

    recieved->message = message;
    recieved->type = type;

    return recieved;
}
