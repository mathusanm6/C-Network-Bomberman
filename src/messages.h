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

game_action *deserialize_game_action(const char*action);

#endif // MESSAGES_CLIENT_H
