#include <cpu/smp.h>
#include <debug/debug.h>
#include <errno.h>
#include <mm/vmm.h>
#include <sched/sched.h>
#include <sched/syscall.h>
#include <sys/isr.h>
#include <sys/prcb.h>

void syscall_handler(registers_t *reg) {
	cli();
	prcb_return_current_cpu()->running_thread->reg = *reg;
	prcb_return_current_cpu()->running_thread->stack =
		prcb_return_current_cpu()->user_stack;
	prcb_return_current_cpu()->fpu_save(
		prcb_return_current_cpu()->running_thread->fpu_storage);
	sti();

	struct syscall_arguments args = {.syscall_nr = reg->rax,
									 .args0 = reg->rdi,
									 .args1 = reg->rsi,
									 .args2 = reg->rdx,
									 .args3 = reg->r10,
									 .args4 = reg->r8,
									 .args5 = reg->r9,
									 .ret = reg->rax};

	syscall_handle(&args);

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
	struct process *proc = sched_get_running_thread()->mother_proc;
	struct pagemap *target_pagemap = proc->process_pagemap;

	uint64_t kernel_addr = vmm_virt_to_kernel(target_pagemap, user_addr);
	return kernel_addr;
}

bool syscall_helper_copy_to_user(uintptr_t user_addr, void *buffer,
								 size_t count) {
	vmm_switch_pagemap(kernel_pagemap);

	struct process *proc = sched_get_running_thread()->mother_proc;
	struct pagemap *target_pagemap = proc->process_pagemap;

	uint64_t kernel_addr = vmm_virt_to_kernel(target_pagemap, user_addr);

	if (!kernel_addr) {
		errno = EFAULT;
		return false;
	}

	memcpy((void *)kernel_addr, buffer, count);
	vmm_switch_pagemap(target_pagemap);
	return true;
}

bool syscall_helper_copy_from_user(uintptr_t user_addr, void *buffer,
								   size_t count) {
	vmm_switch_pagemap(kernel_pagemap);

	struct process *proc = sched_get_running_thread()->mother_proc;
	struct pagemap *target_pagemap = proc->process_pagemap;

	uint64_t kernel_addr = vmm_virt_to_kernel(target_pagemap, user_addr);

	if (!kernel_addr) {
		errno = EFAULT;
		return false;
	}
	memcpy(buffer, (void *)kernel_addr, count);
	vmm_switch_pagemap(target_pagemap);
	return true;
}