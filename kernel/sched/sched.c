#include <asm/asm.h>
#include <debug/debug.h>
#include <kernel.h>
#include <klibc/elf.h>
#include <klibc/vec.h>
#include <sched/sched.h>
#include <sched/syscall.h>
#include <sys/timer.h>

#define VIRTUAL_STACK_ADDR 0x70000000000

uint8_t elf_ident[4] = {0x7f, 'E', 'L', 'F'};
#define ROUND_UP(__addr, __align) (((__addr) + (__align)-1) & ~((__align)-1))

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

void sched_init(uint64_t args) {
	kprintf("SCHED: Creating kernel thread\n");
	vec_init(&threads);
	vec_init(&processes);
	syscall_register_handler(0, syscall_puts);
	process_create("kernel_tasks", 0, 5000, (uintptr_t)kernel_main, args,
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

void process_create_elf(char *name, uint8_t state, uint64_t runtime,
						uint8_t *binary) {
	spinlock_acquire_or_wait(process_lock);
	struct elf_header *header = (struct elf_header *)binary;
	if (header->type != 2) {
		return;
	}
	if (memcmp((void *)header->identifier, elf_ident, 4)) {
		return;
	}
	struct process *proc = kmalloc(sizeof(struct process));
	strncpy(proc->name, name, 256);
	proc->runtime = runtime;
	proc->state = state;
	proc->pid++;
#if defined(__x86_64__)
	proc->process_pagemap = vmm_new_pagemap();
	// taken from MandelbrotOS
	struct elf_prog_header *prog_header =
		(void *)(binary + header->prog_head_off);
	for (size_t i = 0; i < header->prog_head_count; i++) {
		if (prog_header->type == 1) {
			uint64_t mem =
				(uint64_t)pmm_alloc(ROUND_UP(prog_header->mem_size, PAGE_SIZE));
			for (uintptr_t p = 0;
				 p < ROUND_UP(prog_header->mem_size, PAGE_SIZE); p++) {
				vmm_map_page(proc->process_pagemap, prog_header->virt_addr + p,
							 mem + p, 0b111, 0, 0);
			}
			memset((void *)mem, 0, prog_header->mem_size);
			memcpy((void *)mem,
				   (void *)((uint64_t)binary + prog_header->offset),
				   prog_header->file_size);
		}
		prog_header = (struct elf_prog_header *)((uint8_t *)prog_header +
												 header->prog_head_size);
	}
#endif
	vec_init(&proc->process_threads);
	vec_push(&processes, proc);
	kprintf("ELF entry point at 0x%p\n", header->entry);
	thread_create((uintptr_t)header->entry, 0, 1, proc);
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
	thrd->reg.rsp = (uint64_t)pmm_allocz(STACK_SIZE / PAGE_SIZE);
	if (user) {
		thrd->reg.cs = 0x23;
		thrd->reg.ss = 0x1b;
		for (size_t p = 0; p < STACK_SIZE; p += PAGE_SIZE) {
			vmm_map_page(proc->process_pagemap, VIRTUAL_STACK_ADDR + p,
						 (thrd->reg.rsp) + p, 0b111, 0, 0);
		}
		thrd->reg.rsp = VIRTUAL_STACK_ADDR + STACK_SIZE;
		thrd->kernel_stack = (uint64_t)kmalloc(STACK_SIZE);
		thrd->kernel_stack += STACK_SIZE;
	} else {
		thrd->reg.cs = 0x08;
		thrd->reg.ss = 0x10;
		thrd->reg.rsp += STACK_SIZE;
		thrd->kernel_stack = thrd->reg.rsp;
	}
	thrd->reg.rflags = 0x202;
#endif
	vec_push(&threads, thrd);
	vec_push(&proc->process_threads, thrd);
	spinlock_drop(thread_lock);
}
