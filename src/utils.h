#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include <stdlib.h>

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

#endif // SRC_UTILS_H_