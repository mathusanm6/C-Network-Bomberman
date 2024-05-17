
#include "../src/model.h"
#include "test.h"

void test_add_game(test_info *);

test_info *game_table() {
    test_case cases[1] = {
        QUICK_CASE("Add game", test_add_game),
    };

    return cinta_run_cases("Game table tests", cases, 1);
}

void test_add_game(test_info *info) {
    dimension dim = {30, 32};
    for (int i = 1; i < 1000; i++) {
        init_model(dim, SOLO);
    }

    CINTA_ASSERT(true, info);

    reset_games();
}
