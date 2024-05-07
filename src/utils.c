#include "./utils.h"

#include <ctype.h>
#include <stdio.h>
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

int parse_unsigned_within_bounds(const char *str, unsigned minimum, unsigned maximum) {
    if (!is_integer(str)) {
        return -1;
    }

    unsigned n = atoi(str);

    if (n < minimum || n > maximum) {
        return -1;
    }
    return n;
}

char *convert_adrmdif_into_string(uint16_t adrmdiff_[8]) {
    size_t size_addr_string = sizeof(char) * 8 * 5; // : and \0 are counted
    char *addr_string = malloc(size_addr_string);
    RETURN_NULL_IF_NULL(addr_string);
    int n = 0;
    for (unsigned i = 0; i < 8; i++) {
        int r = sprintf(addr_string + n, "%04X", adrmdiff_[i]);
        if (r < 0) {
            goto exit_freeing_addr_string;
        }
        n += r;
        if (i != 7) {
            int r = sprintf(addr_string + n, "%c", ':');
            if (r < 0) {
                goto exit_freeing_addr_string;
            }
            n += r;
        }
    }
    return addr_string;

exit_freeing_addr_string:
    free(addr_string);
    perror("sprintf addr_string");
    return NULL;
}
