#ifndef DYNARRAY_H
#define DYNARRAY_H

#include "alloc.h"
#include "mem.h"
#include <stddef.h>

#define DYNARRAY_STRUCT(TYPE) \
	struct {                  \
		TYPE *storage;        \
		size_t storage_size;  \
		size_t length;        \
	}

#define DYNARRAY_EXTERN(TYPE, THIS) extern DYNARRAY_STRUCT(TYPE) THIS

#define DYNARRAY_GLOBAL(THIS) typeof(THIS) THIS = {0}

#define DYNARRAY_STATIC(TYPE, THIS) static DYNARRAY_STRUCT(TYPE) THIS = {0}

#define DYNARRAY_NEW(TYPE, THIS) DYNARRAY_STRUCT(TYPE) THIS = {0}

#define DYNARRAY_INIT(THIS, INITIAL_SIZE)                               \
	{                                                                   \
		(THIS).storage_size = INITIAL_SIZE;                             \
		(THIS).storage =                                                \
		  alloc((THIS).storage_size * sizeof(typeof(*(THIS).storage))); \
	}

#define DYNARRAY_DEL(THIS) \
	{ free((THIS).storage); }

#define DYNARRAY_GROW(THIS)                                                   \
	{                                                                         \
		if ((THIS).storage == NULL) {                                         \
			DYNARRAY_INIT(THIS, 1);                                           \
		} else {                                                              \
			(THIS).storage_size *= 2;                                         \
			(THIS).storage =                                                  \
			  realloc((THIS).storage,                                         \
					  (THIS).storage_size * sizeof(typeof(*(THIS).storage))); \
		}                                                                     \
	}

#define DYNARRAY_PUSHBACK(THIS, ITEM)             \
	{                                             \
		if ((THIS).length >= (THIS).storage_size) \
			DYNARRAY_GROW(THIS);                  \
		(THIS).storage[(THIS).length++] = ITEM;   \
	}

#endif
