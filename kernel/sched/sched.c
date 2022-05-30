#include <asm/asm.h>
#include <debug/debug.h>
#include <kernel.h>
#include <klibc/vec.h>
#include <sched/sched.h>
#include <sys/timer.h>
#include <sched/syscall.h>

#define VIRTUAL_STACK_ADDR 0x70000000000

lock_t sched_lock;
bool sched_runit = false;

thread_vec_t threads;
process_vec_t processes;

uint64_t pid = -1;
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

void syscall_putchar(struct syscall_arguments *args) {
	kputchar(args->args0);
}

void sched_init(void) {
	kprintf("SCHED: Creating kernel thread\n");
	vec_init(&threads);
	vec_init(&processes);
	syscall_register_handler(0, syscall_putchar);
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
	proc->pid++;
#if defined(__x86_64__)
	if (user)
		proc->process_pagemap = vmm_new_pagemap();
	else
		proc->process_pagemap = &kernel_pagemap;
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
	thrd->lock = 0;
	thrd->mother_proc = proc;
#if defined(__x86_64__)
	thrd->reg.rip = pc_address;
	thrd->reg.rdi = arguments;
	thrd->reg.rsp = (uint64_t)kmalloc(STACK_SIZE);
	thrd->reg.rsp += STACK_SIZE;
	if (user) {
		thrd->reg.cs = 0x23;
		thrd->reg.ss = 0x1b;
		for (size_t p = 0; p < STACK_SIZE; p += PAGE_SIZE) {
			vmm_map_page(proc->process_pagemap, VIRTUAL_STACK_ADDR + p, thrd->reg.rsp + p, 0b111, 0, 0);
		}
		thrd->reg.rsp = VIRTUAL_STACK_ADDR + STACK_SIZE;
		thrd->kernel_stack = (uint64_t)kmalloc(STACK_SIZE);
		thrd->kernel_stack += STACK_SIZE;
	}
	else {
		thrd->reg.cs = 0x08;
		thrd->reg.ss = 0x10;
		thrd->kernel_stack = thrd->reg.rsp;
#define VIRTUAL_STACK_ADDR 0x70000000000
	}
	thrd->reg.rflags = 0x202;
#endif
	vec_push(&threads, thrd);
	vec_push(&proc->process_threads, thrd);
	spinlock_drop(thread_lock);
}
