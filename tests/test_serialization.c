#include <stdlib.h>

#include "../src/messages.h"
#include "test.h"

#define NUMBER_TESTS 9

void test_initial_connection_solo(test_info *info);
void test_initial_connection_team(test_info *info);
void test_initial_connection_invalid_deserialization(test_info *info);
void test_initial_connection_invalid_serialization(test_info *info);

void test_ready_connection_solo(test_info *info);
void test_ready_connection_team(test_info *info);
void test_ready_connection_invalid_game_mode(test_info *info);
void test_ready_connection_invalid_team_number(test_info *info);
void test_ready_connection_invalid_id(test_info *info);

test_info *serialization() {
    test_case cases[NUMBER_TESTS] = {
        QUICK_CASE("De/Serializing initial connection in SOLO mode", test_initial_connection_solo),
        QUICK_CASE("De/Serializing initial connection in TEAM mode", test_initial_connection_team),
        QUICK_CASE("Deserializing invalid initial connection", test_initial_connection_invalid_deserialization),
        QUICK_CASE("Serializing invalid initial connection", test_initial_connection_invalid_serialization),
        QUICK_CASE("De/Serializing ready connection in SOLO mode", test_ready_connection_solo),
        QUICK_CASE("De/Serializing ready connection in TEAM mode", test_ready_connection_team),
        QUICK_CASE("De/Serializing invalid ready connection with invalid game mode",
                   test_ready_connection_invalid_game_mode),
        QUICK_CASE("De/Serializing invalid ready connection with invalid team number",
                   test_ready_connection_invalid_team_number),
        QUICK_CASE("De/Serializing invalid ready connection with invalid id", test_ready_connection_invalid_id)

    };

    return cinta_run_cases("Serialization tests", cases, NUMBER_TESTS);
}

void test_initial_connection_solo(test_info *info) {
    initial_connection_header *header = malloc(sizeof(initial_connection_header));
    header->game_mode = SOLO;
    connection_header_raw *connection = serialize_initial_connection(header);
    initial_connection_header *deserialized = deserialize_initial_connection(connection);
    CINTA_ASSERT(header->game_mode == deserialized->game_mode, info);
    free(header);
    free(connection);
    free(deserialized);
}

void test_initial_connection_team(test_info *info) {
    initial_connection_header *header = malloc(sizeof(initial_connection_header));
    header->game_mode = TEAM;
    connection_header_raw *connection = serialize_initial_connection(header);
    initial_connection_header *deserialized = deserialize_initial_connection(connection);
    CINTA_ASSERT(header->game_mode == deserialized->game_mode, info);
    free(header);
    free(connection);
    free(deserialized);
}

void test_initial_connection_invalid_deserialization(test_info *info) {
    connection_header_raw *connection = malloc(sizeof(connection_header_raw));
    connection->req = 3;
    initial_connection_header *deserialized = deserialize_initial_connection(connection);
    CINTA_ASSERT_NULL(deserialized, info);
    free(connection);
    free(deserialized);
}

void test_initial_connection_invalid_serialization(test_info *info) {
    initial_connection_header *header = malloc(sizeof(initial_connection_header));
    header->game_mode = 3;
    connection_header_raw *connection = serialize_initial_connection(header);
    CINTA_ASSERT_NULL(connection, info);
    free(header);
    free(connection);
}

void test_ready(GAME_MODE game_mode, test_info *info) {
    ready_connection_header *header = malloc(sizeof(ready_connection_header));
    header->game_mode = game_mode;
    for (int i = 0; i < 4; i++) {
        header->id = i;

        for (int j = 0; j < 2; j++) {
            header->eq = j;

            connection_header_raw *connection = serialize_ready_connection(header);
            ready_connection_header *deserialized = deserialize_ready_connection(connection);
            CINTA_ASSERT(header->game_mode == deserialized->game_mode, info);
            CINTA_ASSERT(header->eq == deserialized->eq, info);
            CINTA_ASSERT(header->id == deserialized->id, info);
            free(connection);
            free(deserialized);
        }
    }
    free(header);
}

void test_ready_connection_solo(test_info *info) {
    test_ready(SOLO, info);
}

void test_ready_connection_team(test_info *info) {
    test_ready(TEAM, info);
}

void test_ready_connection_invalid_game_mode(test_info *info) {
    ready_connection_header *header = malloc(sizeof(ready_connection_header));
    header->game_mode = 3;
    header->id = 0;
    header->eq = 0;
    connection_header_raw *connection = serialize_ready_connection(header);
    CINTA_ASSERT_NULL(connection, info);
    free(header);
    free(connection);

    connection_header_raw *connection2 = malloc(sizeof(connection_header_raw));
    connection2->req = 5;
    ready_connection_header *deserialized = deserialize_ready_connection(connection2);
    CINTA_ASSERT_NULL(deserialized, info);
    free(connection2);
    free(deserialized);
}

void test_ready_connection_invalid_team_number(test_info *info) {
    ready_connection_header *header = malloc(sizeof(ready_connection_header));
    header->game_mode = TEAM;
    header->id = 0;
    header->eq = 3;
    connection_header_raw *connection = serialize_ready_connection(header);
    CINTA_ASSERT_NULL(connection, info);
    free(header);
    free(connection);

    connection_header_raw *connection2 = malloc(sizeof(connection_header_raw));
    connection2->req = 3 << 14;
    ready_connection_header *deserialized = deserialize_ready_connection(connection2);
    CINTA_ASSERT_NULL(deserialized, info);
    free(connection2);
    free(deserialized);
}

void test_ready_connection_invalid_id(test_info *info) {
    ready_connection_header *header = malloc(sizeof(ready_connection_header));
    header->game_mode = TEAM;
    header->id = 4;
    header->eq = 0;
    connection_header_raw *connection = serialize_ready_connection(header);
    CINTA_ASSERT_NULL(connection, info);
    free(header);
    free(connection);

    connection_header_raw *connection2 = malloc(sizeof(connection_header_raw));
    connection2->req = 4 << 12;
    ready_connection_header *deserialized = deserialize_ready_connection(connection2);
    CINTA_ASSERT_NULL(deserialized, info);
    free(connection2);
    free(deserialized);
}
