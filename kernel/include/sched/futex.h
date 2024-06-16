#ifndef FUTEX_H
#define FUTEX_H

#include <sched/sched_types.h>

struct futex_entry {
	uint32_t value;
	uint32_t *address;
	struct thread *thrd;
	struct event *event;
};

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1
#define FUTEX_WAIT_BITSET 9
#define FUTEX_WAKE_BITSET 10

void futex_init(void);

#endif
