#include <asm/asm.h>
#include <debug/debug.h>
#include <kernel.h>
#include <klibc/vec.h>
#include <sched/sched.h>
#include <sys/prcb.h>
#include <sys/timer.h>

lock_t sched_lock;
bool sched_runit = false;

thread_vec_t threads;
process_vec_t processes;

uint64_t pid = 0;
lock_t process_lock;

lock_t thread_lock;
uint64_t tid = 0;

int sched_get_next_thread(int index) {
	if (index == -1) {
		index = 0;
	} else {
		index++;
	}

	for (int i = 0; i < threads.length; i++) {
		if (index >= threads.length) {
			index = 0;
		}
		struct thread *thread = threads.data[index];
		if (spinlock_acquire(thread->lock))
			return index;
		index++;
	}

	return -1;
}

void sched_init(void) {
	kprintf("SCHED: Creating kernel thread\n");
	vec_init(&threads);
	vec_init(&processes);
	process_create("kernel_tasks", 0, 5000, (uintptr_t)kernel_main, 0xbabe650,
				   0);
}

void process_create(char *name, uint8_t state, uint64_t runtime,
					uintptr_t pc_address, uint64_t arguments, bool user) {
	spinlock_acquire_or_wait(process_lock);
	struct process *proc = kmalloc(sizeof(struct process));
	strncpy(proc->name, name, 256);
	proc->runtime = runtime;
	proc->state = state;
	proc->pid = pid++;
#if defined(__x86_64__)
	proc->process_pagemap = kernel_pagemap;
#endif
	vec_init(&proc->process_threads);
	vec_push(&processes, proc);
	thread_create(pc_address, arguments, user, proc);
	spinlock_drop(process_lock);
}

void thread_create(uintptr_t pc_address, uint64_t arguments, bool user,
				   struct process *proc) {
	spinlock_acquire(thread_lock);
	struct thread *thrd = kmalloc(sizeof(struct thread));
	thrd->tid = tid++;
	thrd->state = proc->state;
	thrd->runtime = proc->runtime;
#if defined(__x86_64__)
	thrd->reg.rip = pc_address;
	thrd->reg.rdi = arguments;
	thrd->reg.rsp = (uint64_t)kmalloc(STACK_SIZE);
	thrd->page_fault_stack = (uint64_t)kmalloc(STACK_SIZE);
	thrd->reg.rsp += STACK_SIZE;
	thrd->page_fault_stack += STACK_SIZE;
	if (user) {
		thrd->reg.cs = 0x23;
		thrd->reg.ss = 0x1b;
	} else {
		thrd->reg.cs = 0x08;
		thrd->reg.ss = 0x10;
	}
	thrd->reg.rflags = 0x202;
#endif
	thrd->mother_proc = proc;
	thrd->kernel_stack = thrd->reg.rsp;
	vec_push(&threads, thrd);
	vec_push(&proc->process_threads, thrd);
	spinlock_drop(thread_lock);
}
