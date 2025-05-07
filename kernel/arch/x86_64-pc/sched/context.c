#include <debug/debug.h>
#include <klibc/misc.h>
#include <sched/sched.h>
#include <sys/prcb.h>

void thread_setup_context(struct thread *thrd, uintptr_t pc_address,
						  uint64_t arguments, bool user) {
	uint64_t flags = 0;
	asm volatile("pushfq; pop %0" : "=rm"(flags));
	bool old_state = flags & (1 << 9);
	cli();
	thrd->reg.rip = pc_address;
	thrd->reg.rdi = arguments;
	thrd->kernel_stack = ((uint64_t)pmm_allocz(CPU_STACK_SIZE / PAGE_SIZE) +
						  MEM_PHYS_OFFSET + CPU_STACK_SIZE);
	thrd->fpu_storage =
		(void *)((uintptr_t)pmm_allocz(DIV_ROUNDUP(
					 prcb_return_current_cpu()->fpu_storage_size, PAGE_SIZE)) +
				 MEM_PHYS_OFFSET);
	thrd->pf_stack = ((uint64_t)pmm_allocz(CPU_STACK_SIZE / PAGE_SIZE) +
					  MEM_PHYS_OFFSET + CPU_STACK_SIZE);

	struct process *proc = thrd->mother_proc;

	if (user) {
		thrd->reg.cs = 0x23;
		thrd->reg.ss = 0x1b;
		thrd->reg.rsp = (uint64_t)pmm_allocz(STACK_SIZE / PAGE_SIZE);
		thrd->stack = thrd->reg.rsp;

		mmap_range(proc->process_pagemap, proc->stack_top - STACK_SIZE,
				   (uintptr_t)thrd->reg.rsp, STACK_SIZE, PROT_READ | PROT_WRITE,
				   MAP_ANONYMOUS);

		thrd->reg.rsp = proc->stack_top;
		proc->stack_top -= STACK_SIZE;

		prcb_return_current_cpu()->fpu_restore(thrd->fpu_storage);
		uint16_t default_fcw = 0b1100111111;
		asm volatile("fldcw %0" ::"m"(default_fcw) : "memory");
		uint32_t default_mxcsr = 0b1111110000000;
		asm volatile("ldmxcsr %0" ::"m"(default_mxcsr) : "memory");
		prcb_return_current_cpu()->fpu_save(thrd->fpu_storage);

		thrd->fs_base = 0;
		thrd->gs_base = 0;
	} else {
		thrd->reg.cs = 0x08;
		thrd->reg.ss = 0x10;

		thrd->stack = thrd->kernel_stack;
		thrd->reg.rsp = thrd->stack;

		thrd->fs_base = read_fs_base();
		thrd->gs_base = read_kernel_gs();
	}

	thrd->reg.rflags = 0x202;
	if (old_state) {
		sti();
	} else {
		cli();
	}
}

void thread_setup_context_from_user(struct thread *thrd, uintptr_t pc_address,
									uintptr_t sp) {
	uint64_t flags = 0;
	asm volatile("pushfq; pop %0" : "=rm"(flags));
	bool old_state = flags & (1 << 9);
	cli();
	thrd->reg.rip = pc_address;
	thrd->kernel_stack = ((uint64_t)pmm_allocz(CPU_STACK_SIZE / PAGE_SIZE) +
						  MEM_PHYS_OFFSET + CPU_STACK_SIZE);
	thrd->fpu_storage =
		(void *)((uintptr_t)pmm_allocz(DIV_ROUNDUP(
					 prcb_return_current_cpu()->fpu_storage_size, PAGE_SIZE)) +
				 MEM_PHYS_OFFSET);

	struct process *proc = thrd->mother_proc;
	thrd->reg.cs = 0x23;
	thrd->reg.ss = 0x1b;
	thrd->reg.rsp = sp;
	thrd->stack = thrd->reg.rsp;

	thrd->pf_stack = ((uint64_t)pmm_allocz(CPU_STACK_SIZE / PAGE_SIZE) +
					  MEM_PHYS_OFFSET + CPU_STACK_SIZE);

	prcb_return_current_cpu()->fpu_restore(thrd->fpu_storage);
	uint16_t default_fcw = 0b1100111111;
	asm volatile("fldcw %0" ::"m"(default_fcw) : "memory");
	uint32_t default_mxcsr = 0b1111110000000;
	asm volatile("ldmxcsr %0" ::"m"(default_mxcsr) : "memory");
	prcb_return_current_cpu()->fpu_save(thrd->fpu_storage);

	thrd->fs_base = 0;
	thrd->gs_base = 0;

	thrd->reg.rflags = 0x202;
	if (old_state) {
		sti();
	} else {
		cli();
	}
}

// I am still surprised that I still know how this works.
// I will properly document it sometime soon.
void thread_setup_context_for_execve(struct thread *thrd, uintptr_t pc_address,
									 char **argv, char **envp) {
	uint64_t flags = 0;
	asm volatile("pushfq; pop %0" : "=rm"(flags));
	bool old_state = flags & (1 << 9);
	cli();
	struct process *proc = thrd->mother_proc;

	thrd->reg.rip = pc_address;
	thrd->reg.rsp = (uint64_t)pmm_allocz(STACK_SIZE / PAGE_SIZE);
	thrd->stack = thrd->reg.rsp;
	thrd->reg.cs = 0x23;
	thrd->reg.ss = 0x1b;

	mmap_range(proc->process_pagemap, proc->stack_top - STACK_SIZE,
			   (uintptr_t)thrd->reg.rsp, STACK_SIZE + 1, PROT_READ | PROT_WRITE,
			   MAP_ANONYMOUS);

	thrd->reg.rsp = proc->stack_top;

	thrd->kernel_stack = ((uint64_t)pmm_allocz(CPU_STACK_SIZE / PAGE_SIZE) +
						  MEM_PHYS_OFFSET + CPU_STACK_SIZE);
	thrd->pf_stack = ((uint64_t)pmm_allocz(CPU_STACK_SIZE / PAGE_SIZE) +
					  MEM_PHYS_OFFSET + CPU_STACK_SIZE);

	thrd->reg.rflags = 0x202;

	thrd->fpu_storage =
		(void *)((uint64_t)pmm_allocz(DIV_ROUNDUP(
					 prcb_return_current_cpu()->fpu_storage_size, PAGE_SIZE)) +
				 MEM_PHYS_OFFSET);

	prcb_return_current_cpu()->fpu_restore(thrd->fpu_storage);
	uint16_t default_fcw = 0b1100111111;
	asm volatile("fldcw %0" ::"m"(default_fcw) : "memory");
	uint32_t default_mxcsr = 0b1111110000000;
	asm volatile("ldmxcsr %0" ::"m"(default_mxcsr) : "memory");
	prcb_return_current_cpu()->fpu_save(thrd->fpu_storage);

	thrd->fs_base = 0;
	thrd->gs_base = 0;

	struct auxval auxv = proc->auxv;

	// the stack structure.
	// the address values are not accurate.
	/*
	 * 	0x70000000000 - "USER=root\0" 	// envp[0][9]
	 * 	0x6fffffffff6 - proc->name 		// argv[0][255]
	 *	0x6fffffffef6 - 0x0, 0x0		// zeros
	 *	0x6fffffffee6 - AT_ENTRY
	 *	0x6fffffffede - 0x400789		// example values
	 *	0x6fffffffed6 - AT_PHDR
	 *	0x6fffffffece - 2
	 *	0x6fffffffec6 - AT_PHENT
	 *	0x6fffffffebe - 7
	 *	0x6fffffffeb6 - AT_PHNUM
	 *	0x6fffffffeae - 5
	 *	0x6fffffffea6 - 0x0 			// START OF ENVP
	 *	0x6fffffffe9e - 0x6fffffffff6	// pointer to envp[0]
	 *  0x6fffffffe96 - 0x0				// START OF ARGV
	 *	0x6fffffffe8e - 0x6fffffffef6	// pointer to argv[0]
	 *  0x6fffffffe86 - 1				// argc
	 */

	uint64_t *stack = (uint64_t *)(thrd->stack + STACK_SIZE + MEM_PHYS_OFFSET);

	int envp_len = 0;
	uint64_t address_difference = 0;

	uint8_t *stack_but_in_bytes = (uint8_t *)stack;

	for (envp_len = 0; envp[envp_len] != NULL; envp_len++) {
		stack_but_in_bytes -= (strlen(envp[envp_len]) + 1);
		memcpy((void *)stack_but_in_bytes, envp[envp_len],
			   strlen(envp[envp_len]) + 1);
	}

	stack = (uint64_t *)stack_but_in_bytes;
	address_difference =
		(thrd->stack + STACK_SIZE) - ((uint64_t)stack - MEM_PHYS_OFFSET);
	uint64_t addr_to_env = (uint64_t)proc->stack_top - address_difference;

	int argv_len;
	for (argv_len = 0; argv[argv_len] != NULL; argv_len++) {
		stack_but_in_bytes -= (strlen(argv[argv_len]) + 1);
		memcpy((void *)stack_but_in_bytes, argv[argv_len],
			   strlen(argv[argv_len]) + 1);
	}

	stack = (uint64_t *)stack_but_in_bytes;
	address_difference =
		(thrd->stack + STACK_SIZE) - ((uint64_t)stack - MEM_PHYS_OFFSET);
	uint64_t addr_to_arg = (uint64_t)proc->stack_top - address_difference;

	// alignments

	stack = (uintptr_t *)ALIGN_DOWN((uintptr_t)stack, 16);
	if (((argv_len + envp_len + 1) & 1) != 0)
		stack--;

	*(--stack) = 0;
	*(--stack) = 0;
	stack -= 2;
	stack[0] = 23; // AT_SECURE
	stack[1] = 0;
	stack -= 2;
	stack[0] = 9;
	stack[1] = auxv.at_entry;
	stack -= 2;
	stack[0] = 3;
	stack[1] = auxv.at_phdr;
	stack -= 2;
	stack[0] = 4;
	stack[1] = auxv.at_phent;
	stack -= 2;
	stack[0] = 5;
	stack[1] = auxv.at_phnum;

	*(--stack) = 0;

	stack -= envp_len;

	uint64_t offset = 0;
	for (int i = envp_len - 1; i >= 0; i--) {
		if (i != envp_len - 1) {
			offset += strlen(envp[i + 1]) + 1;
		}
		stack[i] = addr_to_env + offset;
	}

	*(--stack) = 0;

	stack -= argv_len;

	offset = 0;
	for (int i = argv_len - 1; i >= 0; i--) {
		if (i != argv_len - 1) {
			offset += strlen(argv[i + 1]) + 1;
		}
		stack[i] = addr_to_arg + offset;
	}

	*(--stack) = argv_len;

	address_difference =
		(thrd->stack + STACK_SIZE) - ((uint64_t)stack - MEM_PHYS_OFFSET);

	thrd->reg.rsp -= address_difference;
	proc->stack_top -= STACK_SIZE;
	if (old_state) {
		sti();
	} else {
		cli();
	}
}

void thread_fork_context(struct thread *thrd, struct thread *fthrd) {
	uint64_t flags = 0;
	asm volatile("pushfq; pop %0" : "=rm"(flags));
	bool old_state = flags & (1 << 9);
	cli();

	fthrd->kernel_stack = ((uint64_t)pmm_allocz(CPU_STACK_SIZE / PAGE_SIZE) +
						   MEM_PHYS_OFFSET + CPU_STACK_SIZE);
	fthrd->pf_stack = ((uint64_t)pmm_allocz(CPU_STACK_SIZE / PAGE_SIZE) +
					   MEM_PHYS_OFFSET + CPU_STACK_SIZE);
	fthrd->reg = thrd->reg;
	fthrd->reg.rax = 0;
	fthrd->reg.rbx = 0;
	fthrd->fs_base = thrd->fs_base;
	fthrd->gs_base = thrd->gs_base;
	fthrd->fpu_storage =
		(void *)((uintptr_t)pmm_allocz(DIV_ROUNDUP(
					 prcb_return_current_cpu()->fpu_storage_size, PAGE_SIZE)) +
				 MEM_PHYS_OFFSET);

	memcpy(fthrd->fpu_storage, thrd->fpu_storage,
		   prcb_return_current_cpu()->fpu_storage_size);
	if (old_state) {
		sti();
	} else {
		cli();
	}
}

void thread_destroy_context(struct thread *thrd) {
	uint64_t flags = 0;
	asm volatile("pushfq; pop %0" : "=rm"(flags));
	bool old_state = flags & (1 << 9);
	cli();
	pmm_free((void *)(thrd->kernel_stack - MEM_PHYS_OFFSET - CPU_STACK_SIZE),
			 CPU_STACK_SIZE / PAGE_SIZE);
	pmm_free((void *)(thrd->pf_stack - MEM_PHYS_OFFSET - CPU_STACK_SIZE),
			 CPU_STACK_SIZE / PAGE_SIZE);
	pmm_free(
		(void *)((uint64_t)thrd->fpu_storage - MEM_PHYS_OFFSET),
		DIV_ROUNDUP(prcb_return_current_cpu()->fpu_storage_size, PAGE_SIZE));
	if (old_state) {
		sti();
	} else {
		cli();
	}
}

void process_setup_context(struct process *proc, bool user) {
	if (user) {
		proc->process_pagemap = vmm_new_pagemap();
	} else {
		proc->process_pagemap = kernel_pagemap;
	}
}

void process_fork_context(struct process *proc, struct process *fproc) {
	fproc->process_pagemap = vmm_fork_pagemap(proc->process_pagemap);
}

void process_destroy_context(struct process *proc) {
	(void)proc;
	// We are killing the running proc time to switch
	uint64_t flags = 0;
	asm volatile("pushfq; pop %0" : "=rm"(flags));
	bool old_state = flags & (1 << 9);
	cli();
	if (prcb_return_current_cpu()->running_thread == NULL) {
		vmm_switch_pagemap(kernel_pagemap);
	}
	if (old_state) {
		sti();
	} else {
		cli();
	}
	vmm_destroy_pagemap(proc->process_pagemap);
}