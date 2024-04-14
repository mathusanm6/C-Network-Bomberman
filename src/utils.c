#include "utils.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

int min(int a, int b) {
    return a < b ? a : b;
}

int max(int a, int b) {
    return a > b ? a : b;
}

bool is_integer(const char *str) {
    for (size_t i = 0; i < strlen(str); ++i) {
        if (!isdigit(str[i])) {
            return false;
        }
    }
    return true;
}

/** Returns -1 in case of error, since the minimum is necessarily greater than or equal to 0
 */
int bounded_nat_of_string(const char *str, unsigned minimum, unsigned maximum) {
    if (!is_integer(str)) {
        return -1;
    }

    unsigned n = atoi(str);

    if (n < minimum || n > maximum) {
        return -1;
    }
    return n;
}
