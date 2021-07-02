#ifndef BITMAN_H
#define BITMAN_H

#include <stddef.h>
#include <stdbool.h>
#include "asm.h"

static inline bool bitmap_test(void *bitmap, size_t bit) {
    bool ret;
    asm volatile (
        "bt %1, %2"
        : "=@ccc" (ret)
        : "m" (FLAT_PTR(bitmap)), "r" (bit)
        : "memory"
    );
    return ret;
}

static inline bool bitmap_set(void *bitmap, size_t bit) {
    bool ret;
    asm volatile (
        "bts %1, %2"
        : "=@ccc" (ret), "+m" (FLAT_PTR(bitmap))
        : "r" (bit)
        : "memory"
    );
    return ret;
}

static inline bool bitmap_unset(void *bitmap, size_t bit) {
    bool ret;
    asm volatile (
        "btr %1, %2"
        : "=@ccc" (ret), "+m" (FLAT_PTR(bitmap))
        : "r" (bit)
        : "memory"
    );
    return ret;
}

#endif
