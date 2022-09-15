#ifndef FUTEX_H
#define FUTEX_H

#include <sched/sched_types.h>

struct futex_entry {
	uint32_t value;
	uint32_t *address;
	struct thread *thrd;
};

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1
#define FUTEX_WAIT_BITSET 9
#define FUTEX_WAKE_BITSET 10

typedef vec_t(struct futex_entry *) futex_entry_t;

void futex_init(void);
bool futex_wait(uint32_t value, uint32_t *futex, struct thread *thrd);
bool futex_wake(uint32_t *futex);

#endif
