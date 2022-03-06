#include <debug/debug.h>
#include <kernel.h>
#include <sched/sched.h>
#include <sys/prcb.h>

void end_thread(void) {
	panic("End of kernel\n");
}

void another_process(void) {
	kprintf("Hello I am %s\n",
			prcb_return_current_cpu()->running_thread->mother_proc->name);
	thread_create((uintptr_t)end_thread, 0, 0,
				  prcb_return_current_cpu()->running_thread->mother_proc);
	for (;;)
		;
}

void kernel_main(void *args) {
	kprintf("Got args 0x%x\n", args);
	kprintf("Hello I am %s\n",
			prcb_return_current_cpu()->running_thread->mother_proc->name);
	process_create("another_kernel_task", 0, 5000, (uintptr_t)another_process,
				   0, 0);
	for (;;)
		;
}
