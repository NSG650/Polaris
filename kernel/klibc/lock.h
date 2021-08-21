#ifndef LOCK_H
#define LOCK_H

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

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	uint32_t waiting_refcount;
	uint32_t bits;
} lock_t;

#define LOCKED_READ(VAR)                                                    \
	({                                                                      \
		typeof(VAR) ret = 0;                                                \
		asm volatile("lock xadd %1, %0" : "+r"(ret) : "m"(VAR) : "memory"); \
		ret;                                                                \
	})

#define LOCKED_WRITE(VAR, VAL)                                              \
	({                                                                      \
		typeof(VAR) ret = VAL;                                              \
		asm volatile("lock xchg %1, %0" : "+r"(ret) : "m"(VAR) : "memory"); \
		ret;                                                                \
	})

#define LOCKED_INC(VAR)                                                    \
	({                                                                     \
		bool ret;                                                          \
		asm volatile("lock inc %1" : "=@ccnz"(ret) : "m"(VAR) : "memory"); \
		ret;                                                               \
	})

#define LOCKED_DEC(VAR)                                                    \
	({                                                                     \
		bool ret;                                                          \
		asm volatile("lock dec %1" : "=@ccnz"(ret) : "m"(VAR) : "memory"); \
		ret;                                                               \
	})

#define SPINLOCK_ACQUIRE(LOCK)                         \
	({                                                 \
		bool ret;                                      \
		LOCKED_INC((LOCK).waiting_refcount);           \
		asm volatile("1: lock bts %0, 0\n\t"           \
					 "jnc 1f\n\t"                      \
					 "bt %0, 1\n\t"                    \
					 "jc 1f\n\t"                       \
					 "pause\n\t"                       \
					 "jmp 1b\n\t"                      \
					 "1:"                              \
					 : "+m"((LOCK).bits), "=@ccc"(ret) \
					 :                                 \
					 : "memory");                      \
		LOCKED_DEC((LOCK).waiting_refcount);           \
		!ret;                                          \
	})

#define LOCK_ACQUIRE(LOCK)                             \
	({                                                 \
		bool ret;                                      \
		asm volatile("lock bts %0, 0"                  \
					 : "+m"((LOCK).bits), "=@ccc"(ret) \
					 :                                 \
					 : "memory");                      \
		!ret;                                          \
	})

#define LOCK_RELEASE(LOCK) \
	({ asm volatile("lock btr %0, 0" : "+m"((LOCK).bits) : : "memory"); })

// XXX only to use on acquired locks
#define LOCK_DESTROY(LOCK)                                               \
	({                                                                   \
		asm volatile("lock bts %0, 1" : "+m"((LOCK).bits) : : "memory"); \
		while (LOCKED_READ((LOCK).waiting_refcount))                     \
			;                                                            \
	})

#endif
