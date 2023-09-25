#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <stdbool.h>

typedef struct {
	bool lock;
	void *last_owner;
	uint64_t last_cpu;
} lock_t;

#define spinlock_init(s) \
	s.lock = 0;          \
	s.last_owner = NULL; \
	s.last_cpu = 0

bool spinlock_acquire(lock_t *spin);
void spinlock_acquire_or_wait(lock_t *spin);
void spinlock_drop(lock_t *spin);

#endif
