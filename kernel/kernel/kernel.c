#include <debug/debug.h>
#include <kernel.h>
#include <sched/thread.h>
#include <sys/prcb.h>

void another_thread(void) {
	kprintf("Hello I am another thread running on CPU%d\n",
			prcb_return_current_cpu()->cpu_number);
	panic("End of kernel\n");
}

void kernel_thread(void *args) {
	kprintf("Hello Kernel Thread!\n");
	kprintf("Got args 0x%x\n", args);
	kprintf("Running on CPU%d\n", prcb_return_current_cpu()->cpu_number);
	thread_create((uintptr_t)another_thread, 0);
	for (;;)
		;
}
