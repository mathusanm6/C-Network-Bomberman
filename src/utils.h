#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include <stdbool.h>
#include <stdlib.h>

#define INT_STRING_SIZE 12

#define RETURN_NULL_IF_NULL(ptr)                                                                                       \
    if (ptr == NULL) {                                                                                                 \
        return NULL;                                                                                                   \
    }

#define RETURN_NULL_IF_NULL_PERROR(ptr, msg)                                                                           \
    if (ptr == NULL) {                                                                                                 \
        perror(msg);                                                                                                   \
        return NULL;                                                                                                   \
    }

#define RETURN_IF_NULL_PTR(ptr)                                                                                        \
    if (ptr == NULL) {                                                                                                 \
        return;                                                                                                        \
    }

#define RETURN_IF_NULL_PTR_PERROR(ptr, msg)                                                                            \
    if (ptr == NULL) {                                                                                                 \
        perror(msg);                                                                                                   \
        return;                                                                                                        \
    }

#define RETURN_FAILURE_IF_NULL(ptr)                                                                                    \
    if (ptr == NULL) {                                                                                                 \
        return EXIT_FAILURE;                                                                                           \
    }

#define RETURN_FAILURE_IF_NULL_PERROR(ptr, msg)                                                                        \
    if (ptr == NULL) {                                                                                                 \
        perror(msg);                                                                                                   \
        return EXIT_FAILURE;                                                                                           \
    }

#define RETURN_FAILURE_IF_ERROR(expr)                                                                                  \
    if (expr == EXIT_FAILURE) {                                                                                        \
        return EXIT_FAILURE;                                                                                           \
    }

#define RETURN_IF_ERROR(expr)                                                                                          \
    if (expr == EXIT_FAILURE) {                                                                                        \
        return;                                                                                                        \
    }

int min(int, int);
int max(int, int);

/** Returns -1 in case of error, since the minimum is necessarily greater than or equal to 0
 */
int bounded_nat_of_string(const char *, unsigned, unsigned);

#endif // SRC_UTILS_H_
