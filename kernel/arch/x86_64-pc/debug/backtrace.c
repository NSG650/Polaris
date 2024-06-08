#include <debug/debug.h>
#include <klibc/module.h>
#include <mm/vmm.h>
#include <sys/elf.h>

void backtrace(uintptr_t *bp) {
	kprintf("Stack trace:\n");
	uintptr_t *rbp = (uintptr_t *)bp;

	if (rbp == NULL)
		asm volatile("mov %%rbp, %0" : "=g"(rbp)::"memory");

	if (rbp == NULL || (uintptr_t)(rbp) < MEM_PHYS_OFFSET)
		return;

	for (;;) {
		uintptr_t *old_rbp = (uintptr_t *)rbp[0];
		uintptr_t *rip = (uintptr_t *)rbp[1];

		if (rip == NULL || old_rbp == NULL ||
			((uintptr_t)rip) < MEM_PHYS_OFFSET)
			break;

		kprintf("%p\n", rip);

		rbp = old_rbp;
	}

	kprintf("Kernel base: %p Mem phys base: %p\n", KERNEL_BASE,
			MEM_PHYS_OFFSET);

	module_dump();
}

void backtrace_unsafe(uintptr_t *bp) {
	kprintf("Stack trace:\n");
	uintptr_t *rbp = (uintptr_t *)bp;

	if (rbp == NULL)
		asm volatile("mov %%rbp, %0" : "=g"(rbp)::"memory");

	if (rbp == NULL)
		return;

	for (;;) {
		uintptr_t *old_rbp = (uintptr_t *)rbp[0];
		uintptr_t *rip = (uintptr_t *)rbp[1];

		if (rip == NULL || old_rbp == NULL)
			break;

		kprintf("%p\n", rip);

		rbp = old_rbp;
	}
}
