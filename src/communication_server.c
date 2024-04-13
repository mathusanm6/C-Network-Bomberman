#include "communication_server.h"

#include <stdio.h>
#include <stdlib.h>
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
    if (head == NULL) {
        perror("malloc connection_information");
        return EXIT_FAILURE;
    }
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

    if (serialized_head == NULL) {
        return EXIT_FAILURE;
    }
    int res = send_connexion_information_raw(sock, serialized_head);
    free(serialized_head);

    return res;
}

connection_header_raw *recv_connexion_header_raw(int sock) {
    connection_header_raw *head = malloc(sizeof(connection_header_raw));
    if (head == NULL) {
        perror("malloc connection_header_raw");
        return NULL;
    }
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
    if (head == NULL) {
        return NULL;
    }
    initial_connection_header *deserialized_head = deserialize_initial_connection(head);
    free(head);
    return deserialized_head;
}

ready_connection_header *recv_ready_connexion_header(int sock) {
    connection_header_raw *head = recv_connexion_header_raw(sock);
    if (head == NULL) {
        return NULL;
    }
    ready_connection_header *deserialized_head = deserialize_ready_connection(head);
    free(head);
    return deserialized_head;
}
