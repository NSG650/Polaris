#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <stdbool.h>

typedef volatile bool lock_t;

bool spinlock_acquire(lock_t *spin);
void spinlock_acquire_or_wait(lock_t *spin);
void spinlock_drop(lock_t *spin);

#endif
