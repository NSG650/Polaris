#ifndef dynarray_H
#define dynarray_H

/*
 * Copyright 2021, 2022 NSG650
 * Copyright 2021, 2022 Sebastian
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stddef.h>
#include <mem/liballoc.h>

#define dynarray_struct(TYPE) \
    struct {                  \
        TYPE  *storage;       \
        size_t storage_size;  \
        size_t length;        \
    }

#define dynarray_extern(TYPE, THIS) \
    extern dynarray_struct(TYPE) THIS

#define dynarray_global(THIS) \
    typeof(THIS) THIS = {0}

#define dynarray_static(TYPE, THIS) \
    static dynarray_struct(TYPE) THIS = {0}

#define dynarray_new(TYPE, THIS) \
    dynarray_struct(TYPE) THIS = {0}

#define dynarray_init(THIS, INITIAL_SIZE) ({                                       \
    (THIS).storage_size = INITIAL_SIZE;                                            \
    (THIS).storage = kmalloc((THIS).storage_size * sizeof(typeof(*(THIS).storage))); \
})

#define dynarray_delete(THIS) ({ \
    kfree((THIS).storage);     \
})

#define dynarray_increase(THIS) ({                                                           \
    if ((THIS).storage == NULL) {                                                        \
        dynarray_init(THIS, 1);                                                          \
    } else {                                                                             \
        (THIS).storage_size *= 2;                                                        \
        (THIS).storage = krealloc((THIS).storage,                                         \
                                 (THIS).storage_size * sizeof(typeof(*(THIS).storage))); \
    }                                                                                    \
})

#define dynarray_push(THIS, ITEM) ({      \
    if ((THIS).length >= (THIS).storage_size) \
        dynarray_increase(THIS);                  \
    (THIS).storage[(THIS).length++] = ITEM;   \
    (THIS).length - 1;                        \
})

#define dynarray_insert(THIS, ITEM) ({     \
    bool found = false;                    \
    size_t i;                              \
    for (i = 0; i < (THIS).length; i++) {  \
        if ((THIS).storage[i] == NULL) {   \
            (THIS).storage[i] = ITEM;      \
            found = true;                  \
            break;                         \
        }                                  \
    }                                      \
    if (found == false) {                  \
        i = dynarray_push(THIS, ITEM); \
    }                                      \
    i;                                     \
})

#define dynarray_insert_at(THIS, ITEM, AT) ({         \
    if ((AT) >= (THIS).storage_size) {                \
        (THIS).storage_size = (AT) + 1;               \
        (THIS).storage =                              \
            krealloc((THIS).storage,                   \
                    (THIS).storage_size               \
                  * sizeof(typeof(*(THIS).storage))); \
    }                                                 \
    (THIS).storage[(AT)] = (ITEM);                    \
    if ((AT) >= (THIS).length)                        \
        (THIS).length = (AT) + 1;                     \
    (AT);                                             \
})

#define dynarray_get_index_by_value(THIS, VALUE) ({ \
    size_t i;                                      \
    for (i = 0; i < (size_t)(THIS).length; i++) {  \
        if ((THIS).storage[i] == (VALUE))           \
            break;                                  \
    }                                               \
    if (i == (size_t)((THIS).length))              \
        i = -1;                                     \
    i;                                              \
})

#define dynarray_remove_by_value(THIS, VALUE) ({   \
    size_t i;                                     \
    for (i = 0; i < (size_t)(THIS).length; i++) { \
        if ((THIS).storage[i] == (VALUE)) {        \
            (THIS).storage[i] = NULL;              \
            break;                                 \
        }                                          \
    }                                              \
    if (i == (size_t)((THIS).length))             \
        i = -1;                                    \
    i;                                             \
})

#define dynarray_remove_by_value_and_pack(THIS, VALUE) ({    \
    size_t i;                                               \
    for (i = 0; i < (size_t)(THIS).length; i++) {           \
        if ((THIS).storage[i] == (VALUE)) {                  \
            for (size_t j = i + 1; j < (THIS).length; j++) { \
                (THIS).storage[j-1] = (THIS).storage[j];     \
            }                                                \
            break;                                           \
        }                                                    \
    }                                                        \
    if (i == (size_t)((THIS).length))                       \
        i = -1;                                              \
    i;                                                       \
})

#define dynarray_remove_and_pack(THIS, INDEX) ({           \
    for (size_t i = (INDEX) + 1; i < (THIS).length; i++) { \
        (THIS).storage[i-1] = (THIS).storage[i];           \
    }                                                      \
    --(THIS).length;                                       \

#endif
