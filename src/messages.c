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
    RETURN_NULL_IF_NULL_PERROR(raw, "malloc");

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
    RETURN_NULL_IF_NULL_PERROR(game_action_, "malloc");

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
    RETURN_NULL_IF_NULL_PERROR(serialized, "malloc");

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
    RETURN_NULL_IF_NULL_PERROR(game_board_info, "malloc");

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
    RETURN_NULL_IF_NULL_PERROR(serialized, "malloc");

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
    RETURN_NULL_IF_NULL_PERROR(game_board_update_, "malloc");

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

static char *serialize_chat_message(const chat_message *message, int initial_codereq) {
    // 2 for the header and 1 for the message length (2+1=3)
    char *serialized = malloc(3 + message->message_length);
    RETURN_NULL_IF_NULL_PERROR(serialized, "malloc");

    int codereq = 0;

    if (message->type == GLOBAL_M) {
        codereq = initial_codereq;
    } else if (message->type == TEAM_M) {
        codereq = initial_codereq + 1;
    } else {
        free(serialized);
        return NULL;
    }

    if (message->id < 0 || message->id > 3) {
        free(serialized);
        return NULL;
    }

    if (message->type == TEAM_M && (message->eq < 0 || message->eq > 1)) {
        free(serialized);
        return NULL;
    }

    uint16_t header = connection_header_value(codereq, message->id, message->eq);

    // Split into 2 bytes
    serialized[0] = header & 0xFF;
    serialized[1] = header >> 8;

    serialized[2] = message->message_length;

    strncpy(serialized + 3, message->message, message->message_length);

    return serialized;
}

#define CLIENT_CHAT_CODE 7
#define SERVER_CHAT_CODE 13

char *client_serialize_chat_message(const chat_message *message) {
    return serialize_chat_message(message, CLIENT_CHAT_CODE);
}

char *server_serialize_chat_message(const chat_message *message) {
    return serialize_chat_message(message, SERVER_CHAT_CODE);
}

chat_message *deserialize_chat_message(const char *message, int initial_codereq) {
    chat_message *chat_message_ = malloc(sizeof(chat_message));
    RETURN_NULL_IF_NULL_PERROR(chat_message_, "malloc");

    uint16_t header = ntohs(*(uint16_t *)message);

    if ((header & BIT_OFFSET_13) == initial_codereq) {
        chat_message_->type = GLOBAL_M;
    } else if ((header & BIT_OFFSET_13) == initial_codereq + 1) {
        chat_message_->type = TEAM_M;
    } else {
        free(chat_message_);
        return NULL;
    }

    chat_message_->id = (header >> 12) & 0x3; // We only need 2 bits
    chat_message_->eq = (header >> 14) & 0x1; // We only need 1 bit

    chat_message_->message_length = message[2];

    chat_message_->message = malloc(chat_message_->message_length + 1);
    if (chat_message_->message == NULL) {
        free(chat_message_);
        return NULL;
    }

    strncpy(chat_message_->message, message + 3, chat_message_->message_length);
    chat_message_->message[chat_message_->message_length] = '\0';

    return chat_message_;
}

chat_message *client_deserialize_chat_message(const char *message) {
    return deserialize_chat_message(message, CLIENT_CHAT_CODE);
}

chat_message *server_deserialize_chat_message(const char *message) {
    return deserialize_chat_message(message, SERVER_CHAT_CODE);
}
