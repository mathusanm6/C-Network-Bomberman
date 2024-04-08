#include "messages.h"
#include "model.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define BIT_OFFSET_13 ((1 << 12) - 1)

// TODO: ENDIANNESS

uint16_t create_connection_header_raw(int codereq, int id, int team_number) {
    return (team_number << 14) + (id << 12) + codereq;
}

connection_header_raw *create_connection_header(int codereq, int id, int team_number) {
    connection_header_raw *connection_req = malloc(sizeof(connection_header_raw));
    if (connection_req == NULL) {
        return NULL;
    }

    connection_req->req = create_connection_header_raw(codereq, id, team_number);

    return connection_req;
}

connection_header_raw *serialize_initial_connection(const initial_connection_header *header) {
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

initial_connection_header *deserialize_initial_connection(const connection_header_raw *header) {
    initial_connection_header *initial_connection = malloc(sizeof(initial_connection_header));
    if (initial_connection == NULL) {
        return NULL;
    }

    switch (header->req & BIT_OFFSET_13) {
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

connection_header_raw *serialize_ready_connection(const ready_connection_header *header) {
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
ready_connection_header *deserialize_ready_connection(const connection_header_raw *header) {
    ready_connection_header *ready_connection = malloc(sizeof(ready_connection_header));
    if (ready_connection == NULL) {
        return NULL;
    }

    switch (header->req & BIT_OFFSET_13) {
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

connection_information_raw *serialize_connection_information(const connection_information *info) {
    int codereq = 1;
    switch (info->game_mode) {
        case SOLO:
            codereq = 9;
            break;
        case TEAM:
            codereq = 10;
            break;
        default:
            return NULL;
    }

    if (info->id < 0 || info->id > 3) {
        return NULL;
    }

    if (info->eq < 0 || info->eq > 1) {
        return NULL;
    }

    connection_information_raw *raw = malloc(sizeof(connection_information_raw));

    if (raw == NULL) {
        return NULL;
    }

    raw->header = create_connection_header_raw(codereq, info->id, info->eq);

    raw->portudp = info->portudp;
    raw->portmdiff = info->portmdiff;
    raw->adrmdiff = info->adrmdiff;

    return raw;
}

connection_information *deserialize_connection_information(const connection_information_raw *info) {

    connection_information *connection_info = malloc(sizeof(connection_information));
    if (connection_info == NULL) {
        return NULL;
    }

    switch (info->header & BIT_OFFSET_13) {
        case 9:
            connection_info->game_mode = SOLO;
            break;
        case 10:
            connection_info->game_mode = TEAM;
            break;
        default:
            free(connection_info);
            return NULL;
    }

    connection_info->id = (info->header >> 12) & 0x3; // We only need 2 bits
    connection_info->eq = (info->header >> 14) & 0x1; // We only need 1 bit

    connection_info->portudp = info->portudp;
    connection_info->portmdiff = info->portmdiff;
    connection_info->adrmdiff = info->adrmdiff;

    return connection_info;
}
