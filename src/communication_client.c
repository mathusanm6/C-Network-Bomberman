#include "communication_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

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
    if (head == NULL) {
        perror("malloc initial connection header");
        return EXIT_FAILURE;
    }
    head->game_mode = mode;

    connection_header_raw *serialized_head = serialize_initial_connection(head);
    free(head);

    if (serialized_head == NULL) {
        return EXIT_FAILURE;
    }
    int res = send_connexion_header_raw(sock, serialized_head);
    free(serialized_head);

    return res;
}

int send_ready_connexion_information(int sock, GAME_MODE mode, int id, int team_number) {
    ready_connection_header *head = malloc(sizeof(ready_connection_header));
    if (head == NULL) {
        perror("malloc ready connection header");
        return EXIT_FAILURE;
    }
    head->game_mode = mode;
    head->id = id;
    head->eq = team_number;

    connection_header_raw *serialized_head = serialize_ready_connection(head);
    free(head);

    if (serialized_head == NULL) {
        return EXIT_FAILURE;
    }
    int res = send_connexion_header_raw(sock, serialized_head);
    free(serialized_head);

    return res;
}

connection_information_raw *recv_connexion_information_raw(int sock) {
    connection_information_raw *head = malloc(sizeof(connection_information_raw));
    if (head == NULL) {
        perror("malloc connection_information_raw");
        return NULL;
    }
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
    if (head == NULL) {
        return NULL;
    }
    connection_information *deserialized_head = deserialize_connection_information(head);
    free(head);
    return deserialized_head;
}
