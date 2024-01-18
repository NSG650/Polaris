#include <debug/debug.h>
#include <mm/vmm.h>

void backtrace(uintptr_t *bp) {
	kprintf("Kernel base: 0x%p Mem phys base: 0x%p\n", KERNEL_BASE,
			MEM_PHYS_OFFSET);

	kprintf("Stack trace:\n");
	uintptr_t *rbp = (uintptr_t *)bp;

	if (rbp == NULL)
		asm volatile("mov %%rbp, %0" : "=g"(rbp)::"memory");

	if (rbp == NULL)
		return;

	for (;;) {
		uintptr_t *old_rbp = (uintptr_t *)rbp[0];
		uintptr_t *rip = (uintptr_t *)rbp[1];

		if (rip == NULL || old_rbp == NULL ||
			((uintptr_t)rip) < 0xffffffff80000000)
			break;

		kprintf("0x%p\n", rip);

		rbp = old_rbp;
	}
}
