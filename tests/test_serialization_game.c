#include <stdlib.h>

#include "../src/messages.h"
#include "test.h"

void test_solo_game_action(test_info *info);
void test_team_game_action(test_info *info);
void test_invalid_game_action_game_mode(test_info *info);
void test_invalid_game_action_id(test_info *info);
void test_game_action_ignores_eq_on_solo(test_info *info);
void test_invalid_game_action_eq(test_info *info);
void test_invalid_game_action_message_number(test_info *info);
void test_invalid_game_action_action(test_info *info);

void test_small_board(test_info *info);
void test_medium_board(test_info *info);
void test_big_board(test_info *info);
void test_invalid_header(test_info *info);
void test_invalid_board(test_info *info);

void test_game_board_update_small(test_info *info);
void test_game_board_update_medium(test_info *info);
void test_game_board_update_large(test_info *info);
void test_game_board_update_invalid_header(test_info *info);
void test_game_board_update_invalid_diff(test_info *info);

void test_game_end_solo(test_info *info);
void test_game_end_team(test_info *info);
void test_invalid_game_end_game_mode(test_info *info);
void test_invalid_game_end_id(test_info *info);
void test_team_ignore_id(test_info *info);
void test_invalid_game_end_eq(test_info *info);
void test_solo_ignores_eq(test_info *info);

#define NUMBER_TESTS 25

test_info *serialization_game() {
    test_case cases[NUMBER_TESTS] = {
        QUICK_CASE("Test solo game action", test_solo_game_action),
        QUICK_CASE("Test team game action", test_team_game_action),
        QUICK_CASE("Test invalid game action game mode", test_invalid_game_action_game_mode),
        QUICK_CASE("Test invalid game action id", test_invalid_game_action_id),
        QUICK_CASE("Test game action ignores eq on solo", test_game_action_ignores_eq_on_solo),
        QUICK_CASE("Test invalid game action eq", test_invalid_game_action_eq),
        QUICK_CASE("Test invalid game action message number", test_invalid_game_action_message_number),
        QUICK_CASE("Test invalid game action action", test_invalid_game_action_action),

        QUICK_CASE("Test small board", test_small_board),
        QUICK_CASE("Test medium board", test_medium_board),
        QUICK_CASE("Test big board", test_big_board),
        QUICK_CASE("Test invalid header", test_invalid_header),
        QUICK_CASE("Test invalid board", test_invalid_board),

        QUICK_CASE("Test game board update small", test_game_board_update_small),
        QUICK_CASE("Test game board update medium", test_game_board_update_medium),
        QUICK_CASE("Test game board update large", test_game_board_update_large),
        QUICK_CASE("Test game board update invalid header", test_game_board_update_invalid_header),
        QUICK_CASE("Test game board update invalid diff", test_game_board_update_invalid_diff),

        QUICK_CASE("Test game end solo", test_game_end_solo),
        QUICK_CASE("Test game end team", test_game_end_team),
        QUICK_CASE("Test invalid game end game mode", test_invalid_game_end_game_mode),
        QUICK_CASE("Test invalid game end id", test_invalid_game_end_id),
        QUICK_CASE("Test team ignore id", test_team_ignore_id),
        QUICK_CASE("Test invalid game end eq", test_invalid_game_end_eq),
        QUICK_CASE("Test solo ignores eq", test_solo_ignores_eq),
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
                CINTA_ASSERT_INT(action->eq, deserialized->eq, info);
                CINTA_ASSERT_INT(action->id, deserialized->id, info);
                CINTA_ASSERT_INT(action->message_number, deserialized->message_number, info);
                CINTA_ASSERT_INT(action->action, deserialized->action, info);

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
    CINTA_ASSERT_NULL(serialized, info);

    free(action);
}

void test_invalid_game_action_eq(test_info *info) {
    game_action *action = malloc(sizeof(game_action));
    action->game_mode = TEAM;
    action->eq = 2;
    action->id = 0;
    action->message_number = 0;
    action->action = GAME_UP;

    char *serialized = serialize_game_action(action);
    CINTA_ASSERT_NULL(serialized, info);

    free(action);
}

void test_game_action_ignores_eq_on_solo(test_info *info) {
    game_action *action = malloc(sizeof(game_action));
    action->game_mode = SOLO;
    action->eq = 2;
    action->id = 0;
    action->message_number = 0;
    action->action = GAME_UP;

    char *serialized = serialize_game_action(action);
    game_action *deserialized = deserialize_game_action(serialized);

    CINTA_ASSERT_INT(deserialized->eq, 0, info);
    CINTA_ASSERT_INT(deserialized->game_mode, action->game_mode, info);
    CINTA_ASSERT_INT(deserialized->id, action->id, info);

    free(action);
    free(deserialized);
    free(serialized);
}

void test_invalid_game_action_id(test_info *info) {
    game_action *action = malloc(sizeof(game_action));
    action->game_mode = SOLO;
    action->eq = 0;
    action->id = 4;
    action->message_number = 0;
    action->action = GAME_UP;

    char *serialized = serialize_game_action(action);
    CINTA_ASSERT_NULL(serialized, info);

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
    CINTA_ASSERT_NULL(serialized, info);

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
    CINTA_ASSERT_NULL(serialized, info);

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

    CINTA_ASSERT_INT(game_info->num, deserialized->num, info);
    CINTA_ASSERT_INT(game_info->height, deserialized->height, info);
    CINTA_ASSERT_INT(game_info->width, deserialized->width, info);

    for (int i = 0; i < height * width; i++) {
        CINTA_ASSERT_INT(game_info->board[i], deserialized->board[i], info);
    }

    free(board);
    free(game_info);
    free(serialized);
    free(deserialized->board);
    free(deserialized);
}

void test_small_board(test_info *info) {
    test_board(1, 1, info, 0, 0);
}

void test_medium_board(test_info *info) {
    test_board(10, 10, info, 0, 0);
}
void test_big_board(test_info *info) {
    test_board(21, 22, info, 0, 0);
}

void test_invalid_header(test_info *info) {
    char *serialized = malloc(sizeof(char) * 16);

    for (int i = 0; i < 16; i++) {
        serialized[i] = 0;
    }

    game_board_information *deserialized = deserialize_game_board(serialized);
    CINTA_ASSERT_NULL(deserialized, info);

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
    CINTA_ASSERT_NULL(serialized, info);

    free(game_info->board);
    free(game_info);
}

void test_update(int nb, test_info *info, int seed, int message_number) {
    game_board_update *update = malloc(sizeof(game_board_update));
    update->num = message_number;
    update->nb = nb;
    update->diff = malloc(sizeof(tile_diff) * nb);

    srandom(seed);

    for (int i = 0; i < nb; i++) {
        update->diff[i].x = rand() % 10;
        update->diff[i].y = rand() % 10;
        update->diff[i].tile = rand() % 9;
    }

    char *serialized = serialize_game_board_update(update);
    game_board_update *deserialized = deserialize_game_board_update(serialized);

    CINTA_ASSERT_INT(update->num, deserialized->num, info);
    CINTA_ASSERT_INT(update->nb, deserialized->nb, info);

    for (int i = 0; i < nb; i++) {
        CINTA_ASSERT_INT(update->diff[i].x, deserialized->diff[i].x, info);
        CINTA_ASSERT_INT(update->diff[i].y, deserialized->diff[i].y, info);
        CINTA_ASSERT_INT(update->diff[i].tile, deserialized->diff[i].tile, info);
    }

    free(serialized);
    free(update->diff);
    free(update);
    free(deserialized->diff);
    free(deserialized);
}

void test_game_board_update_small(test_info *info) {
    test_update(1, info, 0, 0);
}

void test_game_board_update_medium(test_info *info) {
    test_update(10, info, 0, 0);
}

void test_game_board_update_large(test_info *info) {
    test_update(100, info, 0, 0);
}

void test_game_board_update_invalid_header(test_info *info) {
    char *serialized = malloc(sizeof(char) * 16);

    for (int i = 0; i < 16; i++) {
        serialized[i] = 0;
    }

    game_board_update *deserialized = deserialize_game_board_update(serialized);
    CINTA_ASSERT_NULL(deserialized, info);

    free(serialized);
}

void test_game_board_update_invalid_diff(test_info *info) {
    game_board_update *update = malloc(sizeof(game_board_update));
    update->num = 0;
    update->nb = 1;
    update->diff = malloc(sizeof(tile_diff) * 1);

    update->diff[0].x = 0;
    update->diff[0].y = 0;
    update->diff[0].tile = 9;

    char *serialized = serialize_game_board_update(update);
    CINTA_ASSERT_NULL(serialized, info);

    free(update->diff);
    free(update);
}

void test_game_end(GAME_MODE game_mode, test_info *info) {
    game_end *end = malloc(sizeof(game_end));
    end->game_mode = game_mode;

    for (int i = 0; i < 4; i++) {
        end->id = i;

        for (int j = 0; j < 2; j++) {
            end->eq = j;

            char *serialized = serialize_game_end(end);
            game_end *deserialized = deserialize_game_end(serialized);

            CINTA_ASSERT(end->game_mode == deserialized->game_mode, info);
            CINTA_ASSERT_INT(end->id, deserialized->id, info);
            CINTA_ASSERT_INT(end->eq, deserialized->eq, info);

            free(serialized);
            free(deserialized);
        }
    }

    free(end);
}

void test_game_end_solo(test_info *info) {
    test_game_end(SOLO, info);
}

void test_game_end_team(test_info *info) {
    test_game_end(TEAM, info);
}

void test_invalid_game_end_game_mode(test_info *info) {
    game_end *end = malloc(sizeof(game_end));
    end->game_mode = 2;
    end->id = 0;
    end->eq = 0;

    char *serialized = serialize_game_end(end);
    CINTA_ASSERT_NULL(serialized, info);

    free(end);
}

void test_invalid_game_end_id(test_info *info) {
    game_end *end = malloc(sizeof(game_end));
    end->game_mode = SOLO;
    end->id = 5;
    end->eq = 0;

    char *serialized = serialize_game_end(end);
    CINTA_ASSERT_NULL(serialized, info);

    free(end);
}

void test_team_ignore_id(test_info *info) {
    game_end *end = malloc(sizeof(game_end));
    end->game_mode = TEAM;
    end->id = 5;
    end->eq = 1;

    char *serialized = serialize_game_end(end);
    game_end *deserialized = deserialize_game_end(serialized);

    CINTA_ASSERT_INT(deserialized->id, 1, info);
    CINTA_ASSERT(deserialized->game_mode == end->game_mode, info);

    free(end);
    free(deserialized);
    free(serialized);
}

void test_invalid_game_end_eq(test_info *info) {
    game_end *end = malloc(sizeof(game_end));
    end->game_mode = TEAM;
    end->id = 0;
    end->eq = 2;

    char *serialized = serialize_game_end(end);
    CINTA_ASSERT_NULL(serialized, info);

    free(end);
}

void test_solo_ignores_eq(test_info *info) {
    game_end *end = malloc(sizeof(game_end));
    end->game_mode = SOLO;
    end->id = 1;
    end->eq = 2;

    char *serialized = serialize_game_end(end);
    game_end *deserialized = deserialize_game_end(serialized);

    CINTA_ASSERT_INT(deserialized->id, 1, info);
    CINTA_ASSERT(deserialized->game_mode == end->game_mode, info);

    free(end);
    free(deserialized);
    free(serialized);
}
