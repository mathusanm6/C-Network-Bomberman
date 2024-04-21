#include "communication_client.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

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
