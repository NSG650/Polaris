#include <cpu/smp.h>
#include <debug/debug.h>
#include <errno.h>
#include <mm/vmm.h>
#include <sched/sched.h>
#include <sched/syscall.h>
#include <sys/isr.h>
#include <sys/prcb.h>

void syscall_handler(registers_t *reg) {
	// Save original reg in case we're suspending the current thread
	prcb_return_current_cpu()->running_thread->reg = *reg;
	prcb_return_current_cpu()->running_thread->stack =
		prcb_return_current_cpu()->user_stack;
	prcb_return_current_cpu()->fpu_save(
		prcb_return_current_cpu()->running_thread->fpu_storage);

	struct syscall_arguments args = {.syscall_nr = reg->rax,
									 .args0 = reg->rdi,
									 .args1 = reg->rsi,
									 .args2 = reg->rdx,
									 .args3 = reg->r10,
									 .args4 = reg->r8,
									 .args5 = reg->r9,
									 .ret = reg->rax};

	syscall_handle(&args);

	// A user thread must be killed only after it returns back to userspace
	// This is done such that if the user thread is called to be killed while it
	// is in kernel space due to a syscall It can do its entire work in kernel
	// space finally cleaning up resources and freeing locks it held

	if (prcb_return_current_cpu()->running_thread->marked_for_execution) {
		cli();
		// risky put a lock here
		struct process *mother_proc =
			prcb_return_current_cpu()->running_thread->mother_proc;
		vec_remove(&mother_proc->process_threads,
				   prcb_return_current_cpu()->running_thread);
		if (mother_proc->process_threads.length < 1) {
			process_kill(mother_proc, false);
		}
		sched_remove_thread_from_list(
			&thread_list, prcb_return_current_cpu()->running_thread);
		sched_add_thread_to_list(&threads_on_the_death_row,
								 prcb_return_current_cpu()->running_thread);
		sched_resched_now();
	}

	int64_t ret = (int64_t)args.ret;
	if (ret < 0) {
		ret = -((int)errno);
		reg->rax = ret;
	} else
		reg->rax = args.ret;
}

void syscall_install_handler(void) {
	isr_register_handler(0x80, syscall_handler);
}

uint64_t syscall_helper_user_to_kernel_address(uintptr_t user_addr) {
	struct process *proc =
		prcb_return_current_cpu()->running_thread->mother_proc;
	struct pagemap *target_pagemap = proc->process_pagemap;

	uint64_t kernel_addr = vmm_virt_to_kernel(target_pagemap, user_addr);
	return kernel_addr;
}

void syscall_helper_copy_to_user(uintptr_t user_addr, void *buffer,
								 size_t count) {
	vmm_switch_pagemap(kernel_pagemap);

	struct process *proc =
		prcb_return_current_cpu()->running_thread->mother_proc;
	struct pagemap *target_pagemap = proc->process_pagemap;

	uint64_t kernel_addr = vmm_virt_to_kernel(target_pagemap, user_addr);

	if (kernel_addr)
		memcpy((void *)kernel_addr, buffer, count);

	vmm_switch_pagemap(target_pagemap);
}
