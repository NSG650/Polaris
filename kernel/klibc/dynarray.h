#ifndef DYNARRAY_H
#define DYNARRAY_H

/*
 * Copyright 2021 NSG650
 * Copyright 2021 Sebastian
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

#include <liballoc.h>
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

#define DYNARRAY_INIT(THIS, INITIAL_SIZE)                                 \
	{                                                                     \
		(THIS).storage_size = INITIAL_SIZE;                               \
		(THIS).storage =                                                  \
		  kmalloc((THIS).storage_size * sizeof(typeof(*(THIS).storage))); \
	}

#define DYNARRAY_DEL(THIS) \
	{ kfree((THIS).storage); }

#define DYNARRAY_GROW(THIS)                                                    \
	{                                                                          \
		if ((THIS).storage == NULL) {                                          \
			DYNARRAY_INIT(THIS, 1);                                            \
		} else {                                                               \
			(THIS).storage_size *= 2;                                          \
			(THIS).storage =                                                   \
			  krealloc((THIS).storage,                                         \
					   (THIS).storage_size * sizeof(typeof(*(THIS).storage))); \
		}                                                                      \
	}

#define DYNARRAY_PUSHBACK(THIS, ITEM)             \
	{                                             \
		if ((THIS).length >= (THIS).storage_size) \
			DYNARRAY_GROW(THIS);                  \
		(THIS).storage[(THIS).length++] = ITEM;   \
	}

#endif
