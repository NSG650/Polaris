#include <debug/debug.h>
#include <kernel.h>
#include <sched/sched.h>
#include <sys/prcb.h>

void user_thread(void) {
	asm volatile("int 3");
	for (;;)
		;
}

void kernel_main(void *args) {
	thread_create((uintptr_t)user_thread, 0, 1, prcb_return_current_cpu()->running_thread->mother_proc);
	kprintf("Got args 0x%x\n", args);
	kprintf("Hello I am %s\n",
			prcb_return_current_cpu()->running_thread->mother_proc->name);
	for (int i = 0; i < threads.length; i++) 
		kprintf("%d\n", threads.data[i]->tid);
	for (;;)
		;
}
