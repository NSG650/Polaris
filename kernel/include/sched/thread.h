#ifndef THREAD_H
#define THREAD_H

#include <locks/spinlock.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__x86_64__)
#include "../../arch/x86_64/include/reg.h"
#endif

struct thread {
	uint64_t tid;
	registers_t reg;
	lock_t lock;
	uint8_t state;
	uint64_t runtime;
};

#define STACK_SIZE 32768

extern struct thread *threads;
extern uint64_t thread_count;
void thread_create(uintptr_t pc_address, uint64_t arguments);

#endif
