#ifndef SCHED_TYPES_H
#define SCHED_TYPES_H

#include <fs/vfs.h>
#include <klibc/vec.h>
#include <locks/spinlock.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__x86_64__)
#include "../../arch/x86_64/include/mm/vmm.h"
#include "../../arch/x86_64/include/reg.h"
#endif

enum thread_states { THREAD_NORMAL = 0, THREAD_READY_TO_RUN, THREAD_SLEEPING };

enum process_states { PROCESS_NORMAL = 0, PROCESS_READY_TO_RUN };

struct process;

struct thread {
	uint64_t errno;
	int64_t tid;
	registers_t reg;
	lock_t lock;
	enum thread_states state;
	uint64_t runtime;
	uint64_t stack;
	uint64_t kernel_stack;
	uint64_t sleeping_till;
	struct process *mother_proc;
};

typedef vec_t(struct thread *) thread_vec_t;
typedef vec_t(struct process *) process_vec_t;

struct process {
	char name[256];
	int64_t pid;
	struct pagemap *process_pagemap;
	enum process_states state;
	uint64_t runtime;
	thread_vec_t process_threads;
	file_vec_t file_descriptors;
	char cwd[256];
	struct process *parent_process;
	process_vec_t child_processes;
};

#define STACK_SIZE 32768

#endif
