#include "test.h"

#define TEST_NUM 1

test tests[TEST_NUM] = {serialization};

int main(int argc, char *argv[]) {
    return cinta_main(argc, argv, tests, TEST_NUM);
}
