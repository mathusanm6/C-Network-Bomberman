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

#endif // MESSAGES_CLIENT_H
