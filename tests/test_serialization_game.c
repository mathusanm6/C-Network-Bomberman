#include <stdlib.h>

#include "../src/messages.h"
#include "test.h"

void test_solo_game_action(test_info *info);
void test_team_game_action(test_info *info);
void test_invalid_game_action_game_mode(test_info *info);
void test_invalid_game_action_eq(test_info *info);
void test_invalid_game_action_id(test_info *info);
void test_invalid_game_action_message_number(test_info *info);
void test_invalid_game_action_action(test_info *info);

void test_small_board(test_info *info);
void test_medium_board(test_info *info);
void test_invalid_header(test_info *info);
void test_invalid_board(test_info *info);

#define NUMBER_TESTS 11

test_info *serialization_game() {
    test_case cases[NUMBER_TESTS] = {
        QUICK_CASE("Test solo game action", test_solo_game_action),
        QUICK_CASE("Test team game action", test_team_game_action),
        QUICK_CASE("Test invalid game action game mode", test_invalid_game_action_game_mode),
        QUICK_CASE("Test invalid game action eq", test_invalid_game_action_eq),
        QUICK_CASE("Test invalid game action id", test_invalid_game_action_id),
        QUICK_CASE("Test invalid game action message number", test_invalid_game_action_message_number),
        QUICK_CASE("Test invalid game action action", test_invalid_game_action_action),
        QUICK_CASE("Test small board", test_small_board),
        QUICK_CASE("Test medium board", test_medium_board),
        QUICK_CASE("Test invalid header", test_invalid_header),
        QUICK_CASE("Test invalid board", test_invalid_board),
    };

    return cinta_run_cases("Serialization tests | Game", cases, NUMBER_TESTS);
}

void test_game_action(GAME_MODE game_mode, test_info *info, int message_number) {
    game_action *action = malloc(sizeof(game_action));
    action->game_mode = game_mode;
    for (int i = 0; i < 4; i++) {
        action->id = i;

        for (int j = 0; j < 2; j++) {
            action->eq = j;

            for (int k = 0; k < 6; k++) {

                action->message_number = message_number;
                action->action = (GAME_ACTION)k;

                char *serialized = serialize_game_action(action);
                game_action *deserialized = deserialize_game_action(serialized);

                CINTA_ASSERT(action->game_mode == deserialized->game_mode, info);
                CINTA_ASSERT(action->eq == deserialized->eq, info);
                CINTA_ASSERT(action->id == deserialized->id, info);
                CINTA_ASSERT(action->message_number == deserialized->message_number, info);
                CINTA_ASSERT(action->action == deserialized->action, info);

                free(serialized);
                free(deserialized);
            }
        }
    }
    free(action);
}

void test_solo_game_action(test_info *info) {
    test_game_action(SOLO, info, 0);
}

void test_team_game_action(test_info *info) {
    test_game_action(TEAM, info, 0);
}

void test_invalid_game_action_game_mode(test_info *info) {
    game_action *action = malloc(sizeof(game_action));
    action->game_mode = 2;
    action->eq = 0;
    action->id = 0;
    action->message_number = 0;
    action->action = GAME_UP;

    char *serialized = serialize_game_action(action);
    CINTA_ASSERT(serialized == NULL, info);

    free(action);
}

void test_invalid_game_action_eq(test_info *info) {
    game_action *action = malloc(sizeof(game_action));
    action->game_mode = SOLO;
    action->eq = 2;
    action->id = 0;
    action->message_number = 0;
    action->action = GAME_UP;

    char *serialized = serialize_game_action(action);
    CINTA_ASSERT(serialized == NULL, info);

    free(action);
}

void test_invalid_game_action_id(test_info *info) {
    game_action *action = malloc(sizeof(game_action));
    action->game_mode = SOLO;
    action->eq = 0;
    action->id = 4;
    action->message_number = 0;
    action->action = GAME_UP;

    char *serialized = serialize_game_action(action);
    CINTA_ASSERT(serialized == NULL, info);

    free(action);
}

void test_invalid_game_action_message_number(test_info *info) {
    game_action *action = malloc(sizeof(game_action));
    action->game_mode = SOLO;
    action->eq = 0;
    action->id = 0;
    action->message_number = 1 << 18;
    action->action = GAME_UP;

    char *serialized = serialize_game_action(action);
    CINTA_ASSERT(serialized == NULL, info);

    free(action);
}

void test_invalid_game_action_action(test_info *info) {
    game_action *action = malloc(sizeof(game_action));
    action->game_mode = SOLO;
    action->eq = 0;
    action->id = 0;
    action->message_number = 0;
    action->action = 6;

    char *serialized = serialize_game_action(action);
    CINTA_ASSERT(serialized == NULL, info);

    free(action);
}

void test_board(int height, int width, test_info *info, int seed, int message_number) {
    TILE *board = malloc(sizeof(TILE) * height * width);
    srandom(seed);
    for (int i = 0; i < height * width; i++) {
        board[i] = rand() % 9;
    }

    game_board_information *game_info = malloc(sizeof(game_board_information));
    game_info->num = message_number;
    game_info->height = height;
    game_info->width = width;
    game_info->board = board;

    char *serialized = serialize_game_board(game_info);
    game_board_information *deserialized = deserialize_game_board(serialized);

    CINTA_ASSERT(game_info->num == deserialized->num, info);
    CINTA_ASSERT(game_info->height == deserialized->height, info);
    CINTA_ASSERT(game_info->width == deserialized->width, info);

    for (int i = 0; i < height * width; i++) {
        CINTA_ASSERT(game_info->board[i] == deserialized->board[i], info);
    }

    free(serialized);
    free(game_info);
    free(deserialized);
    free(board);
}

void test_small_board(test_info *info) {
    test_board(1, 1, info, 0, 0);
}

void test_medium_board(test_info *info) {
    test_board(10, 10, info, 0, 0);
}

void test_invalid_header(test_info *info) {
    char *serialized = malloc(sizeof(char) * 16);

    for (int i = 0; i < 16; i++) {
        serialized[i] = 0;
    }

    game_board_information *deserialized = deserialize_game_board(serialized);
    CINTA_ASSERT(deserialized == NULL, info);

    free(serialized);
}

void test_invalid_board(test_info *info) {
    game_board_information *game_info = malloc(sizeof(game_board_information));
    game_info->num = 0;
    game_info->height = 1;
    game_info->width = 1;
    game_info->board = malloc(sizeof(TILE) * 1);
    game_info->board[0] = 9;

    char *serialized = serialize_game_board(game_info);
    CINTA_ASSERT(serialized == NULL, info);

    free(game_info->board);
    free(game_info);
}
