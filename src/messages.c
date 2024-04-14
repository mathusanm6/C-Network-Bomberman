#include "messages.h"
#include "model.h"
#include "utils.h"
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BIT_OFFSET_13 ((1 << 12) - 1)

static uint16_t connection_header_value(int codereq, int id, int team_number) {
    return htons((team_number << 14) + (id << 12) + codereq);
}

connection_header_raw *create_connection_header_raw(int codereq, int id, int team_number) {
    connection_header_raw *connection_req = malloc(sizeof(connection_header_raw));
    RETURN_NULL_IF_NULL_PERROR(connection_req, "malloc");

    connection_req->req = connection_header_value(codereq, id, team_number);

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
    return create_connection_header_raw(codereq, 0, 0);
}

initial_connection_header *deserialize_initial_connection(const connection_header_raw *header) {
    initial_connection_header *initial_connection = malloc(sizeof(initial_connection_header));
    RETURN_NULL_IF_NULL_PERROR(initial_connection, "malloc");

    uint16_t req = ntohs(header->req);

    switch (req & BIT_OFFSET_13) {
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

    return create_connection_header_raw(codereq, header->id, header->eq);
}
ready_connection_header *deserialize_ready_connection(const connection_header_raw *header) {
    ready_connection_header *ready_connection = malloc(sizeof(ready_connection_header));
    RETURN_NULL_IF_NULL_PERROR(ready_connection, "malloc");

    uint16_t req = ntohs(header->req);

    switch (req & BIT_OFFSET_13) {
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

    ready_connection->id = (req >> 12) & 0x3; // We only need 2 bits
    ready_connection->eq = (req >> 14) & 0x1; // We only need 1 bit
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
    RETURN_NULL_IF_NULL_PERROR(raw, "malloc");

    raw->header = connection_header_value(codereq, info->id, info->eq);

    raw->portudp = htons(info->portudp);
    raw->portmdiff = htons(info->portmdiff);
    for (int i = 0; i < 8; ++i) {
        raw->adrmdiff[i] = htons(info->adrmdiff[i]);
    }
    return raw;
}

connection_information *deserialize_connection_information(const connection_information_raw *info) {
    connection_information *connection_info = malloc(sizeof(connection_information));
    RETURN_NULL_IF_NULL_PERROR(connection_info, "malloc");

    uint16_t header = ntohs(info->header);

    switch (header & BIT_OFFSET_13) {
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

    connection_info->id = (header >> 12) & 0x3; // We only need 2 bits
    connection_info->eq = (header >> 14) & 0x1; // We only need 1 bit

    connection_info->portudp = ntohs(info->portudp);
    connection_info->portmdiff = ntohs(info->portmdiff);
    for (int i = 0; i < 8; ++i) {
        connection_info->adrmdiff[i] = ntohs(info->adrmdiff[i]);
    }

    return connection_info;
}

uint16_t game_action_value(int messages_num, GAME_ACTION action) {
    return htons(messages_num + (action << 12));
}

char *serialize_game_action(const game_action *action) {
    char *raw = malloc(sizeof(char) * 16 * 2);
    if (raw == NULL) {
        return NULL;
    }

    int codereq = 1;

    switch (action->game_mode) {
        case SOLO:
            codereq = 5;
            break;
        case TEAM:
            codereq = 6;
            break;
        default:
            free(raw);
            return NULL;
    }

    if (action->id < 0 || action->id > 3) {
        free(raw);
        return NULL;
    }

    if (action->eq < 0 || action->eq > 1) {
        free(raw);
        return NULL;
    }

    uint16_t header = connection_header_value(codereq, action->id, action->eq);

    if (action->message_number < 0 || action->message_number > (1 << 13)) {
        free(raw);
        return NULL;
    }

    if (action->action < 0 || action->action > 5) {
        free(raw);
        return NULL;
    }

    uint16_t action_ = game_action_value(action->message_number, action->action);

    memcpy(raw, &header, sizeof(uint16_t));
    memcpy(raw + sizeof(uint16_t), &action_, sizeof(uint16_t));

    return raw;
}

game_action *deserialize_game_action(const char *action) {
    game_action *game_action_ = malloc(sizeof(game_action));
    if (game_action_ == NULL) {
        return NULL;
    }

    uint16_t header;
    uint16_t action_;

    memcpy(&header, action, sizeof(uint16_t));
    memcpy(&action_, action + sizeof(uint16_t), sizeof(uint16_t));

    header = ntohs(header);
    action_ = ntohs(action_);

    switch (header & BIT_OFFSET_13) {
        case 5:
            game_action_->game_mode = SOLO;
            break;
        case 6:
            game_action_->game_mode = TEAM;
            break;
        default:
            free(game_action_);
            return NULL;
    }

    game_action_->id = (header >> 12) & 0x3; // We only need 2 bits
    game_action_->eq = (header >> 14) & 0x1; // We only need 1 bit

    game_action_->message_number = action_ & BIT_OFFSET_13;
    game_action_->action = (action_ >> 12) & 0x7; // We only need 3 bits

    return game_action_;
}
