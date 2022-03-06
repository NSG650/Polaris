#ifndef SCHED_H
#define SCHED_H

#include <stddef.h>
#include <stdint.h>
#include <locks/spinlock.h>
#include <sched/sched_types.h>

#if defined(__x86_64__)
#include "../../arch/x86_64/include/reg.h"
#include "../../arch/x86_64/include/mm/vmm.h"
void resched(registers_t *reg);
#endif

extern process_vec_t processes;
extern thread_vec_t threads;

int sched_get_next_thread(int index);
void sched_init(void);
void process_create(char *name, uint8_t state, uint64_t runtime, uintptr_t pc_address, uint64_t arguments, bool user);
void thread_create(uintptr_t pc_address, uint64_t arguments, bool user, struct process* proc);

#endif
