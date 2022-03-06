#ifndef SCHED_TYPES_H
#define SCHED_TYPES_H

#include <klibc/vec.h>
#include <locks/spinlock.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__x86_64__)
#include "../../arch/x86_64/include/mm/vmm.h"
#include "../../arch/x86_64/include/reg.h"
#endif

struct process;

struct thread {
	uint64_t tid;
	registers_t reg;
	lock_t lock;
	uint8_t state;
	uint64_t runtime;
	struct process *mother_proc;
};

typedef vec_t(struct thread *) thread_vec_t;

struct process {
	char name[256];
	uint64_t pid;
	struct pagemap *process_pagemap;
	uint8_t state;
	uint64_t runtime;
	thread_vec_t process_threads;
};

typedef vec_t(struct process *) process_vec_t;

#define STACK_SIZE 32768

#endif
