#include "messages.h"
#include "model.h"
#include "utils.h"
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BIT_OFFSET_13 ((1 << 12) - 1)
#define BYTE_SIZE 8

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

    if (header->game_mode == TEAM && (header->eq < 0 || header->eq > 1)) {
        return NULL;
    }

    return create_connection_header_raw(codereq, header->id, header->eq & 0x1);
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

uint16_t game_action_value(int message_num, GAME_ACTION action) {
    return htons(message_num + (action << 12));
}

char *serialize_game_action(const game_action *game_action) {
    char *raw = malloc(sizeof(uint16_t) * 2);
    if (raw == NULL) {
        return NULL;
    }

    int codereq = 1;

    switch (game_action->game_mode) {
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

    if (game_action->id < 0 || game_action->id > 3) {
        free(raw);
        return NULL;
    }

    if (game_action->game_mode == TEAM && (game_action->eq < 0 || game_action->eq > 1)) {
        free(raw);
        return NULL;
    }

    uint16_t header = connection_header_value(codereq, game_action->id, game_action->eq);

    if (game_action->message_number < 0 || game_action->message_number >= (1 << 13)) {
        free(raw);
        return NULL;
    }

    if (game_action->action < 0 || game_action->action > 5) {
        free(raw);
        return NULL;
    }

    uint16_t action = game_action_value(game_action->message_number, game_action->action);

    memcpy(raw, &header, sizeof(uint16_t));
    memcpy(raw + sizeof(uint16_t), &action, sizeof(uint16_t));

    return raw;
}

game_action *deserialize_game_action(const char *game_action_raw) {
    game_action *game_action_ = malloc(sizeof(game_action));
    if (game_action_ == NULL) {
        return NULL;
    }

    uint16_t header;
    uint16_t action;

    memcpy(&header, game_action_raw, sizeof(uint16_t));
    memcpy(&action, game_action_raw + sizeof(uint16_t), sizeof(uint16_t));

    header = ntohs(header);
    action = ntohs(action);

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

    game_action_->message_number = action & BIT_OFFSET_13;
    game_action_->action = (action >> 12) & 0x7; // We only need 3 bits

    return game_action_;
}

char *serialize_game_board(const game_board_information *info) {
    // 6 corresponds to the number of bytes of the header, the
    // message number and the height and width of the board
    char *serialized = malloc((info->height * info->width) + 6);
    if (serialized == NULL) {
        return NULL;
    }

    uint16_t header = connection_header_value(11, 0, 0);
    uint16_t num = htons(info->num);

    // Split into 2 bytes
    serialized[0] = header & 0xFF;
    serialized[1] = header >> 8;

    // Split into 2 bytes
    serialized[2] = num & 0xFF;
    serialized[3] = num >> 8;

    serialized[4] = info->height;
    serialized[5] = info->width;

    for (int i = 0; i < info->height * info->width; ++i) {
        if (info->board[i] > 8) {
            free(serialized);
            return NULL;
        }
        serialized[6 + i] = info->board[i];
    }

    return serialized;
}

game_board_information *deserialize_game_board(const char *info) {
    game_board_information *game_board_info = malloc(sizeof(game_board_information));
    if (game_board_info == NULL) {
        return NULL;
    }

    uint16_t header = ntohs(*(uint16_t *)info);

    if ((header & BIT_OFFSET_13) != 11) {
        free(game_board_info);
        return NULL;
    }

    if ((header >> 12) != 0) {
        free(game_board_info);
        return NULL;
    }

    if ((header >> 14) != 0) {
        free(game_board_info);
        return NULL;
    }

    game_board_info->num = ntohs(*(uint16_t *)(info + 2));
    game_board_info->height = info[4];
    game_board_info->width = info[5];

    unsigned size = game_board_info->height * game_board_info->width;

    game_board_info->board = malloc(size * sizeof(TILE));
    if (game_board_info->board == NULL) {
        free(game_board_info);
        return NULL;
    }

    for (unsigned i = 0; i < size; ++i) {
        game_board_info->board[i] = info[6 + i];
    }

    return game_board_info;
}

char *serialize_game_board_update(const game_board_update *update) {
    // 5 corresponds to the number of bytes of the header, the message number
    // and the number of tile_diffs
    char *serialized = malloc((update->nb * 3) + 5);
    if (serialized == NULL) {
        return NULL;
    }

    uint16_t header = connection_header_value(12, 0, 0);
    uint16_t num = htons(update->num);

    // Split into 2 bytes
    serialized[0] = header & 0xFF;
    serialized[1] = header >> 8;

    // Split into 2 bytes
    serialized[2] = num & 0xFF;
    serialized[3] = num >> 8;

    serialized[4] = update->nb;

    for (int i = 0; i < update->nb; ++i) {
        if (update->diff[i].tile > 8) {
            free(serialized);
            return NULL;
        }
        serialized[5 + i * 3] = update->diff[i].x;
        serialized[6 + i * 3] = update->diff[i].y;
        serialized[7 + i * 3] = update->diff[i].tile;
    }

    return serialized;
}

game_board_update *deserialize_game_board_update(const char *update) {
    game_board_update *game_board_update_ = malloc(sizeof(game_board_update));
    if (game_board_update_ == NULL) {
        return NULL;
    }

    uint16_t header = ntohs(*(uint16_t *)update);

    if ((header & BIT_OFFSET_13) != 12) {
        free(game_board_update_);
        return NULL;
    }

    if ((header >> 12) != 0) {
        free(game_board_update_);
        return NULL;
    }

    if ((header >> 14) != 0) {
        free(game_board_update_);
        return NULL;
    }

    game_board_update_->num = ntohs(*(uint16_t *)(update + 2));
    game_board_update_->nb = update[4];

    game_board_update_->diff = malloc(sizeof(tile_diff) * game_board_update_->nb);

    if (game_board_update_->diff == NULL) {
        free(game_board_update_);
        return NULL;
    }

    for (int i = 0; i < game_board_update_->nb; ++i) {
        game_board_update_->diff[i].x = update[5 + i * 3];
        game_board_update_->diff[i].y = update[6 + i * 3];
        game_board_update_->diff[i].tile = update[7 + i * 3];
    }

    return game_board_update_;
}
