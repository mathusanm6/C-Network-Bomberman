#include "test.h"

#define TEST_NUM 2

test tests[TEST_NUM] = {serialization_connection, serialization_game};

int main(int argc, char *argv[]) {
    return cinta_main(argc, argv, tests, TEST_NUM);
}
