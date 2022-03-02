#ifndef SCHED_H
#define SCHED_H

#include <stddef.h>
#include <stdint.h>

#if defined(__x86_64__)
#include "../../arch/x86_64/include/reg.h"
void resched(registers_t *reg);
#endif

int64_t sched_get_next_thread(int64_t index);
void sched_init(void);

#endif
