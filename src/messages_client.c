#include "messages_client.h"
#include "model.h"
#include <stdio.h>
#include <stdlib.h>

connection_header *create_connection_header(int codereq, int id, int team_number) {
    connection_header *connection_req = malloc(sizeof(connection_header));
    if (connection_req == NULL) {
        return NULL;
    }

    connection_req->req = (team_number << 14) + (id << 12) + codereq;

    return connection_req;
}

connection_header *serialize_initial_connection(const initial_connection_header *header) {

    int codereq = 1;
    switch (header->game_mode) {
        case SOLO:
            codereq = 1;
            break;
        case TEAM:
            codereq = 2;
            break;
        default:
            return NULL;
    }
    return create_connection_header(codereq, 0, 0);
}

initial_connection_header *deserialize_initial_connection(const connection_header *header) {
    initial_connection_header *initial_connection = malloc(sizeof(initial_connection_header));
    if (initial_connection == NULL) {
        return NULL;
    }

    switch (header->req & 0x3) { // We only need 2 bits (the maximum value is 2)
        case 1:
            initial_connection->game_mode = SOLO;
            break;
        case 2:
            initial_connection->game_mode = TEAM;
            break;
        default:
            free(initial_connection);
            return NULL;
    }

    return initial_connection;
}

connection_header *serialize_ready_connection(const ready_connection_header *header) {

    int codereq = 1;
    switch (header->game_mode) {
        case SOLO:
            codereq = 3;
            break;
        case TEAM:
            codereq = 4;
            break;
        default:
            return NULL;
    }

    if (header->id < 0 || header->id > 3) {
        return NULL;
    }

    if (header->eq < 0 || header->eq > 1) {
        return NULL;
    }

    return create_connection_header(codereq, header->id, header->eq);
}
ready_connection_header *deserialize_ready_connection(const connection_header *header) {
    ready_connection_header *ready_connection = malloc(sizeof(ready_connection_header));
    if (ready_connection == NULL) {
        return NULL;
    }

    switch (header->req & 0x7) { // We only need 3 bits (the maximum value is 4)
        case 3:
            ready_connection->game_mode = SOLO;
            break;
        case 4:
            ready_connection->game_mode = TEAM;
            break;
        default:
            free(ready_connection);
            return NULL;
    }

    ready_connection->id = (header->req >> 12) & 0x3; // We only need 2 bits
    ready_connection->eq = (header->req >> 14) & 0x1; // We only need 1 bit
    return ready_connection;
}
