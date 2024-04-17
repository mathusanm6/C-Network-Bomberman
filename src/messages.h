#ifndef MESSAGES_CLIENT_H
#define MESSAGES_CLIENT_H

#include "model.h"
#include <stdint.h>

typedef struct connection_header_raw {
    uint16_t req;
} connection_header_raw;

typedef struct initial_connection_header {
    GAME_MODE game_mode;
} initial_connection_header;

connection_header_raw *serialize_initial_connection(const initial_connection_header *header);
initial_connection_header *deserialize_initial_connection(const connection_header_raw *header);

typedef struct ready_connection_header {
    GAME_MODE game_mode;
    int id;
    int eq;
} ready_connection_header;

connection_header_raw *serialize_ready_connection(const ready_connection_header *header);
ready_connection_header *deserialize_ready_connection(const connection_header_raw *header);

typedef struct connection_information_raw {
    uint16_t header;
    uint16_t portudp;
    uint16_t portmdiff;
    uint16_t adrmdiff[8];
} connection_information_raw;

typedef struct connection_information {
    GAME_MODE game_mode;
    int id;
    int eq;
    int portudp;
    int portmdiff;
    uint16_t adrmdiff[8];
} connection_information;

connection_information_raw *serialize_connection_information(const connection_information *info);

connection_information *deserialize_connection_information(const connection_information_raw *info);

typedef struct game_action {
    GAME_MODE game_mode;
    int id;
    int eq;
    int message_number;
    GAME_ACTION action;
} game_action;

char *serialize_game_action(const game_action *action);

game_action *deserialize_game_action(const char *action);

typedef struct game_board_information {
    uint16_t num;
    uint8_t height;
    uint8_t width;
    TILE *board;
} game_board_information;

char *serialize_game_board(const game_board_information *info);

game_board_information *deserialize_game_board(const char *info);

typedef struct tile_diff {
    uint8_t x;
    uint8_t y;
    TILE tile;
} tile_diff;

typedef struct game_board_update {
    uint16_t num;
    uint8_t nb;
    tile_diff *diff;
} game_board_update;

char *serialize_game_board_update(const game_board_update *update);

game_board_update *deserialize_game_board_update(const char *update);

typedef enum chat_message_type { GLOBAL_M, TEAM_M } chat_message_type;

typedef struct chat_message {
    chat_message_type type;
    int id;
    int eq;
    uint8_t message_length;
    char *message; // `\0` terminated
} chat_message;

char *client_serialize_chat_message(const chat_message *message);

chat_message *client_deserialize_chat_message(const char *message);

char *server_serialize_chat_message(const chat_message *message);

chat_message *server_deserialize_chat_message(const char *message);

typedef struct game_end {
    GAME_MODE game_mode;
    int id;
    int eq;
} game_end;

char *serialize_game_end(const game_end *end);

game_end *deserialize_game_end(const char *end);

#endif // MESSAGES_CLIENT_H
