#include <stdint.h>
#include <stdlib.h>

#include "../src/messages.h"
#include "test.h"

#define NUMBER_TESTS 16

void test_initial_connection_solo(test_info *info);
void test_initial_connection_team(test_info *info);
void test_initial_connection_invalid_deserialization(test_info *info);
void test_initial_connection_invalid_serialization(test_info *info);

void test_ready_connection_solo(test_info *info);
void test_ready_connection_team(test_info *info);
void test_ready_connection_invalid_game_mode(test_info *info);
void test_ready_connection_invalid_team_number(test_info *info);
void test_ready_connection_invalid_id(test_info *info);
void test_ready_connection_ignores_eq(test_info *info);
void test_ready_connnection_invalid_eq(test_info *info);

void test_connection_information_solo(test_info *info);
void test_connection_information_team(test_info *info);
void test_connection_information_invalid_game_mode(test_info *info);
void test_connection_information_invalid_team_number(test_info *info);
void test_connection_information_invalid_id(test_info *info);

test_info *serialization_connection() {
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
        QUICK_CASE("De/Serializing invalid ready connection with invalid id", test_ready_connection_invalid_id),
        QUICK_CASE("De/Serializing ready connection solo ignores eq", test_ready_connection_ignores_eq),
        QUICK_CASE("De/Serializing invalid ready connection with invalid eq", test_ready_connnection_invalid_eq),
        QUICK_CASE("De/Serializing connection information in SOLO mode", test_connection_information_solo),
        QUICK_CASE("De/Serializing connection information in TEAM mode", test_connection_information_team),
        QUICK_CASE("De/Serializing invalid connection information with invalid game mode",
                   test_connection_information_invalid_game_mode),
        QUICK_CASE("De/Serializing invalid connection information with invalid team number",
                   test_connection_information_invalid_team_number),
        QUICK_CASE("De/Serializing invalid connection information with invalid id",
                   test_connection_information_invalid_id)};

    return cinta_run_cases("Serialization tests | Connection", cases, NUMBER_TESTS);
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
            CINTA_ASSERT_INT(header->eq, deserialized->eq, info);
            CINTA_ASSERT_INT(header->id, deserialized->id, info);
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

void test_ready_connection_ignores_eq(test_info *info) {
    ready_connection_header *header = malloc(sizeof(ready_connection_header));
    header->game_mode = SOLO;
    header->id = 0;
    header->eq = 3;
    connection_header_raw *connection = serialize_ready_connection(header);
    ready_connection_header *deserialized = deserialize_ready_connection(connection);
    CINTA_ASSERT(header->game_mode == deserialized->game_mode, info);
    CINTA_ASSERT_INT(header->id, deserialized->id, info);
    CINTA_ASSERT_INT(deserialized->eq, 1, info);
    free(header);
    free(connection);
    free(deserialized);
}

void test_ready_connnection_invalid_eq(test_info *info) {
    ready_connection_header *header = malloc(sizeof(ready_connection_header));
    header->game_mode = TEAM;
    header->id = 0;
    header->eq = 3;
    connection_header_raw *connection = serialize_ready_connection(header);
    CINTA_ASSERT_NULL(connection, info);
    free(header);
    free(connection);
}

void test_connection_information(GAME_MODE game_mode, test_info *info, int portudp, int portmdiff, int *adrmdiff) {
    connection_information *header = malloc(sizeof(connection_information));
    header->game_mode = game_mode;
    for (int i = 0; i < 4; i++) {
        header->id = i;

        for (int j = 0; j < 2; j++) {
            header->eq = j;
        }

        header->portudp = portudp;
        header->portmdiff = portmdiff;

        for (int i = 0; i < 8; i++) {
            header->adrmdiff[i] = adrmdiff[i];
        }

        connection_information_raw *connection = serialize_connection_information(header);
        connection_information *deserialized = deserialize_connection_information(connection);
        CINTA_ASSERT(header->game_mode == deserialized->game_mode, info);
        CINTA_ASSERT_INT(header->eq, deserialized->eq, info);
        CINTA_ASSERT_INT(header->id, deserialized->id, info);
        CINTA_ASSERT_INT(header->portudp, deserialized->portudp, info);
        CINTA_ASSERT_INT(header->portmdiff, deserialized->portmdiff, info);
        for (int i = 0; i < 8; i++) {
            CINTA_ASSERT_INT(header->adrmdiff[i], deserialized->adrmdiff[i], info);
        }

        free(connection);
        free(deserialized);
    }
    free(header);
}

static int test_adr[8] = {1, 2, 3, 4, 5, 6, 7, 8};
static uint16_t test_adr_raw[8] = {1, 2, 3, 4, 5, 6, 7, 8};

void test_connection_information_solo(test_info *info) {
    /* TODO: Port numbers and addresses should be better tested */
    test_connection_information(SOLO, info, 1234, 1235, test_adr);
}

void test_connection_information_team(test_info *info) {
    /* TODO: Port numbers and addresses should be better tested */
    test_connection_information(TEAM, info, 1234, 1235, test_adr);
}

void test_connection_information_invalid_game_mode(test_info *info) {
    connection_information *header = malloc(sizeof(connection_information));
    header->game_mode = 3;
    header->id = 0;
    header->eq = 0;
    header->portudp = 1234;
    header->portmdiff = 1235;
    for (int i = 0; i < 8; i++) {
        header->adrmdiff[i] = test_adr[i];
    }
    connection_information_raw *connection = serialize_connection_information(header);
    CINTA_ASSERT_NULL(connection, info);
    free(header);
    free(connection);

    connection_information_raw *connection2 = malloc(sizeof(connection_information_raw));
    connection2->header = 3 << 12;
    connection2->portudp = 1234;
    connection2->portmdiff = 1235;
    for (int i = 0; i < 8; i++) {
        connection2->adrmdiff[i] = test_adr_raw[i];
    }
    connection_information *deserialized = deserialize_connection_information(connection2);
    CINTA_ASSERT_NULL(deserialized, info);
    free(connection2);
    free(deserialized);
}

void test_connection_information_invalid_team_number(test_info *info) {
    connection_information *header = malloc(sizeof(connection_information));
    header->game_mode = TEAM;
    header->id = 0;
    header->eq = 3;
    header->portudp = 1234;
    header->portmdiff = 1235;
    for (int i = 0; i < 8; i++) {
        header->adrmdiff[i] = test_adr[i];
    }
    connection_information_raw *connection = serialize_connection_information(header);
    CINTA_ASSERT_NULL(connection, info);
    free(header);
    free(connection);

    connection_information_raw *connection2 = malloc(sizeof(connection_information_raw));
    connection2->header = 4 << 12;
    connection2->portudp = 1234;
    connection2->portmdiff = 1235;
    for (int i = 0; i < 8; i++) {
        connection2->adrmdiff[i] = test_adr_raw[i];
    }
    connection_information *deserialized = deserialize_connection_information(connection2);
    CINTA_ASSERT_NULL(deserialized, info);
    free(connection2);
    free(deserialized);
}

void test_connection_information_invalid_id(test_info *info) {
    connection_information *header = malloc(sizeof(connection_information));
    header->game_mode = TEAM;
    header->id = 4;
    header->eq = 0;
    header->portudp = 1234;
    header->portmdiff = 1235;
    for (int i = 0; i < 8; i++) {
        header->adrmdiff[i] = test_adr[i];
    }
    connection_information_raw *connection = serialize_connection_information(header);
    CINTA_ASSERT_NULL(connection, info);
    free(header);
    free(connection);

    connection_information_raw *connection2 = malloc(sizeof(connection_information_raw));
    connection2->header = 10 << 12;
    connection2->portudp = 1234;
    connection2->portmdiff = 1235;
    for (int i = 0; i < 8; i++) {
        connection2->adrmdiff[i] = test_adr_raw[i];
    }
    connection_information *deserialized = deserialize_connection_information(connection2);
    CINTA_ASSERT_NULL(deserialized, info);
    free(connection2);
    free(deserialized);
}
