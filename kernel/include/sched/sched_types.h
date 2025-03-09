#ifndef SCHED_TYPES_H
#define SCHED_TYPES_H

#include <fs/vfs.h>
#include <klibc/event.h>
#include <klibc/resource.h>
#include <klibc/vec.h>
#include <locks/spinlock.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/elf.h>

#if defined(__x86_64__)
#include <mm/vmm.h>
#include <reg.h>
#endif

enum thread_states {
	THREAD_NORMAL = 0,
	THREAD_READY_TO_RUN,
	THREAD_SLEEPING,
	THREAD_WAITING_FOR_FUTEX,
	THREAD_WAITING_FOR_EVENT
};

enum process_states {
	PROCESS_NORMAL = 0,
	PROCESS_READY_TO_RUN,
	PROCESS_WAITING_ON_ANOTHER_PROCESS,
	PROCESS_BLOCKED
};

struct process;

#define MAX_FDS 256
#define MAX_EVENTS 32

struct thread {
	uint64_t errno;
	int64_t tid;
	registers_t reg;
	lock_t lock;
	enum thread_states state;
	uint64_t runtime;
	uint64_t stack;
	uint64_t pf_stack;
	uint64_t kernel_stack;
	uint64_t sleeping_till;
	uint64_t last_scheduled;
	void *fpu_storage;
	size_t which_event;
	size_t attached_events_i;
	struct event *attached_events[MAX_EVENTS];
	struct process *mother_proc;
	bool marked_for_execution;
	uint64_t gs_base;
	uint64_t fs_base;
	struct thread *next;
};

struct dead_process {
	int64_t pid;
	uint8_t exit_code;
	bool was_it_killed;
	struct process *parent_process;
};

typedef vec_t(struct dead_process *) dead_process_vec_t;
typedef vec_t(struct thread *) thread_vec_t;
typedef vec_t(struct process *) process_vec_t;

struct process {
	int64_t pid;
	enum process_states state;
	struct pagemap *process_pagemap;
	lock_t lock;
	uintptr_t mmap_anon_base;
	uint64_t runtime;
	uintptr_t stack_top;
	thread_vec_t process_threads;
	struct vfs_node *cwd;
	lock_t fds_lock;
	mode_t umask;
	struct f_descriptor *fds[MAX_FDS];
	struct process *parent_process;
	process_vec_t child_processes;
	struct auxval auxv;
	struct event death_event;
	struct dead_process waitee;
	char name[256];
	struct process *next;
};

#define CPU_STACK_SIZE (32 * 1024)
#define STACK_SIZE (1024 * 1024 * 2)

#endif
