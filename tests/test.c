#include "test.h"

#define TEST_NUM 4

test tests[TEST_NUM] = {serialization_connection, serialization_game, serialization_chat, game_table};

int main(int argc, char *argv[]) {
    return cinta_main(argc, argv, tests, TEST_NUM);
}
