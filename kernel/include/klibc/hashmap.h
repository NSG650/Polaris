#ifndef HASHMAP_H
#define HASHMAP_H

#include <klibc/mem.h>
#include <mm/slab.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// sdbm from: http://www.cse.yorku.ca/~oz/hash.html
static inline uint32_t hash(const void *data, size_t length) {
	const uint8_t *data_u8 = data;
	uint32_t hash = 0;

	for (size_t i = 0; i < length; i++) {
		uint32_t c = data_u8[i];
		hash = c + (hash << 6) + (hash << 16) - hash;
	}

	return hash;
}

#define HASHMAP_KEY_DATA_MAX 256

#define HASHMAP_INIT(CAP) {.cap = (CAP), .buckets = NULL}

#define HASHMAP_DELETE(HASHMAP)                                     \
	do {                                                            \
		__auto_type HASHMAP_DELETE_hashmap = HASHMAP;               \
                                                                    \
		if (HASHMAP_DELETE_hashmap->buckets == NULL) {              \
			break;                                                  \
		}                                                           \
                                                                    \
		for (size_t HASHMAP_DELETE_i = 0;                           \
			 HASHMAP_DELETE_i < HASHMAP_DELETE_hashmap->cap;        \
			 HASHMAP_DELETE_i++) {                                  \
			__auto_type HASHMAP_DELETE_bucket =                     \
				&HASHMAP_DELETE_hashmap->buckets[HASHMAP_DELETE_i]; \
                                                                    \
			kfree(HASHMAP_DELETE_bucket->items);                    \
		}                                                           \
                                                                    \
		kfree(HASHMAP_DELETE_hashmap->buckets);                     \
	} while (0)

#define HASHMAP_TYPE(TYPE)                              \
	struct {                                            \
		size_t cap;                                     \
		struct {                                        \
			size_t cap;                                 \
			size_t filled;                              \
			struct {                                    \
				uint8_t key_data[HASHMAP_KEY_DATA_MAX]; \
				size_t key_length;                      \
				TYPE item;                              \
			} *items;                                   \
		} *buckets;                                     \
	}

#define HASHMAP_GET(HASHMAP, RET, KEY_DATA, KEY_LENGTH)                     \
	({                                                                      \
		__label__ out;                                                      \
		bool HASHMAP_GET_ok = false;                                        \
                                                                            \
		__auto_type HASHMAP_GET_key_data = KEY_DATA;                        \
		__auto_type HASHMAP_GET_key_length = KEY_LENGTH;                    \
                                                                            \
		__auto_type HASHMAP_GET_hashmap = HASHMAP;                          \
		if (HASHMAP_GET_hashmap->buckets == NULL) {                         \
			goto out;                                                       \
		}                                                                   \
                                                                            \
		size_t HASHMAP_GET_hash =                                           \
			hash(HASHMAP_GET_key_data, HASHMAP_GET_key_length);             \
		size_t HASHMAP_GET_index =                                          \
			HASHMAP_GET_hash % HASHMAP_GET_hashmap->cap;                    \
                                                                            \
		__auto_type HASHMAP_GET_bucket =                                    \
			&HASHMAP_GET_hashmap->buckets[HASHMAP_GET_index];               \
                                                                            \
		for (size_t HASHMAP_GET_i = 0;                                      \
			 HASHMAP_GET_i < HASHMAP_GET_bucket->filled; HASHMAP_GET_i++) { \
			if (HASHMAP_GET_key_length !=                                   \
				HASHMAP_GET_bucket->items[HASHMAP_GET_i].key_length) {      \
				continue;                                                   \
			}                                                               \
			if (memcmp(HASHMAP_GET_key_data,                                \
					   HASHMAP_GET_bucket->items[HASHMAP_GET_i].key_data,   \
					   HASHMAP_GET_key_length) == 0) {                      \
				RET = HASHMAP_GET_bucket->items[HASHMAP_GET_i].item;        \
				HASHMAP_GET_ok = true;                                      \
				break;                                                      \
			}                                                               \
		}                                                                   \
                                                                            \
	out:                                                                    \
		HASHMAP_GET_ok;                                                     \
	})

#define HASHMAP_SGET(HASHMAP, RET, STRING)             \
	({                                                 \
		const char *HASHMAP_SGET_string = (STRING);    \
		HASHMAP_GET(HASHMAP, RET, HASHMAP_SGET_string, \
					strlen(HASHMAP_SGET_string));      \
	})

#define HASHMAP_REMOVE(HASHMAP, KEY_DATA, KEY_LENGTH)                        \
	({                                                                       \
		__label__ out;                                                       \
                                                                             \
		bool HASHMAP_REMOVE_ok = false;                                      \
                                                                             \
		__auto_type HASHMAP_REMOVE_key_data = KEY_DATA;                      \
		__auto_type HASHMAP_REMOVE_key_length = KEY_LENGTH;                  \
                                                                             \
		__auto_type HASHMAP_REMOVE_hashmap = HASHMAP;                        \
                                                                             \
		if (HASHMAP_REMOVE_hashmap->buckets == NULL) {                       \
			goto out;                                                        \
		}                                                                    \
                                                                             \
		size_t HASHMAP_REMOVE_hash =                                         \
			hash(HASHMAP_REMOVE_key_data, HASHMAP_REMOVE_key_length);        \
		size_t HASHMAP_REMOVE_index =                                        \
			HASHMAP_REMOVE_hash % HASHMAP_REMOVE_hashmap->cap;               \
                                                                             \
		__auto_type HASHMAP_REMOVE_bucket =                                  \
			&HASHMAP_REMOVE_hashmap->buckets[HASHMAP_REMOVE_index];          \
                                                                             \
		for (size_t HASHMAP_REMOVE_i = 0;                                    \
			 HASHMAP_REMOVE_i < HASHMAP_REMOVE_bucket->filled;               \
			 HASHMAP_REMOVE_i++) {                                           \
			if (HASHMAP_REMOVE_key_length !=                                 \
				HASHMAP_REMOVE_bucket->items[HASHMAP_REMOVE_i].key_length) { \
				continue;                                                    \
			}                                                                \
			if (memcmp(                                                      \
					HASHMAP_REMOVE_key_data,                                 \
					HASHMAP_REMOVE_bucket->items[HASHMAP_REMOVE_i].key_data, \
					HASHMAP_REMOVE_key_length) == 0) {                       \
				if (HASHMAP_REMOVE_i != HASHMAP_REMOVE_bucket->filled - 1) { \
					memcpy(&HASHMAP_REMOVE_bucket->items[HASHMAP_REMOVE_i],  \
						   &HASHMAP_REMOVE_bucket                            \
								->items[HASHMAP_REMOVE_bucket->filled - 1],  \
						   sizeof(*HASHMAP_REMOVE_bucket->items));           \
				}                                                            \
				HASHMAP_REMOVE_bucket->filled--;                             \
				HASHMAP_REMOVE_ok = true;                                    \
				break;                                                       \
			}                                                                \
		}                                                                    \
                                                                             \
	out:                                                                     \
		HASHMAP_REMOVE_ok;                                                   \
	})

#define HASHMAP_SREMOVE(HASHMAP, STRING)                \
	({                                                  \
		const char *HASHMAP_SREMOVE_string = (STRING);  \
		HASHMAP_REMOVE(HASHMAP, HASHMAP_SREMOVE_string, \
					   strlen(HASHMAP_SREMOVE_string)); \
	})

#define HASHMAP_INSERT(HASHMAP, KEY_DATA, KEY_LENGTH, ITEM)                \
	do {                                                                   \
		__auto_type HASHMAP_INSERT_key_data = KEY_DATA;                    \
		__auto_type HASHMAP_INSERT_key_length = KEY_LENGTH;                \
                                                                           \
		__auto_type HASHMAP_INSERT_hashmap = HASHMAP;                      \
		if (HASHMAP_INSERT_hashmap->buckets == NULL) {                     \
			HASHMAP_INSERT_hashmap->buckets =                              \
				kmalloc(HASHMAP_INSERT_hashmap->cap *                      \
						sizeof(*HASHMAP_INSERT_hashmap->buckets));         \
		}                                                                  \
                                                                           \
		size_t HASHMAP_INSERT_hash =                                       \
			hash(HASHMAP_INSERT_key_data, HASHMAP_INSERT_key_length);      \
		size_t HASHMAP_INSERT_index =                                      \
			HASHMAP_INSERT_hash % HASHMAP_INSERT_hashmap->cap;             \
                                                                           \
		__auto_type HASHMAP_INSERT_bucket =                                \
			&HASHMAP_INSERT_hashmap->buckets[HASHMAP_INSERT_index];        \
                                                                           \
		if (HASHMAP_INSERT_bucket->cap == 0) {                             \
			HASHMAP_INSERT_bucket->cap = 16;                               \
			HASHMAP_INSERT_bucket->items =                                 \
				kmalloc(HASHMAP_INSERT_bucket->cap *                       \
						sizeof(*HASHMAP_INSERT_bucket->items));            \
		}                                                                  \
                                                                           \
		if (HASHMAP_INSERT_bucket->filled == HASHMAP_INSERT_bucket->cap) { \
			HASHMAP_INSERT_bucket->cap *= 2;                               \
			HASHMAP_INSERT_bucket->items =                                 \
				krealloc(HASHMAP_INSERT_bucket->items,                     \
						 HASHMAP_INSERT_bucket->cap *                      \
							 sizeof(*HASHMAP_INSERT_bucket->items));       \
		}                                                                  \
                                                                           \
		__auto_type HASHMAP_INSERT_item =                                  \
			&HASHMAP_INSERT_bucket->items[HASHMAP_INSERT_bucket->filled];  \
                                                                           \
		memcpy(HASHMAP_INSERT_item->key_data, HASHMAP_INSERT_key_data,     \
			   HASHMAP_INSERT_key_length);                                 \
		HASHMAP_INSERT_item->key_length = HASHMAP_INSERT_key_length;       \
		HASHMAP_INSERT_item->item = ITEM;                                  \
                                                                           \
		HASHMAP_INSERT_bucket->filled++;                                   \
	} while (0)

#define HASHMAP_SINSERT(HASHMAP, STRING, ITEM)                \
	do {                                                      \
		const char *HASHMAP_SINSERT_string = (STRING);        \
		HASHMAP_INSERT(HASHMAP, HASHMAP_SINSERT_string,       \
					   strlen(HASHMAP_SINSERT_string), ITEM); \
	} while (0)

#endif
