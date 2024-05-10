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
    printf("Receiving header\n");
    uint16_t header;

    struct sockaddr_in6 addr;
    socklen_t addr_len = sizeof(addr);
    int res = recvfrom(info->sock, &header, sizeof(uint16_t), 0, (struct sockaddr *)&info->addr, &addr_len);
    printf("Received %d bytes\n", res);
    RETURN_NULL_IF_NEG_PERROR(res, "recvfrom header");

    return deserialize_message_header(header);
}

void recvfrom_full(const udp_information *info, char *buffer, int size) {
    printf("Receiving %d bytes\n", size);
    int received = 0;
    socklen_t addr_len = sizeof(info->addr);
    while (received < size) {
        int res =
            recvfrom(info->sock, buffer + received, size - received, 0, (struct sockaddr *)&info->addr, &addr_len);
        RETURN_IF_NEG_PERROR(res, "recvfrom_full");

        received += res;
    }
}
