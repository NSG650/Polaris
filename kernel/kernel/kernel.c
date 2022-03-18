#include <debug/debug.h>
#include <kernel.h>
#include <sched/sched.h>
#include <sys/prcb.h>

void user_process(void) {
	for (;;)
		;
}

void kernel_main(void *args) {
	kprintf("Got args 0x%x\n", args);
	kprintf("Hello I am %s\n",
			prcb_return_current_cpu()->running_thread->mother_proc->name);
	kprintf("Spawning user_process\n");
	process_create("user_process", 0, 5000, (uintptr_t)user_process, 0, 1);
	kprintf("Dumping process table\n");
	kprintf("PID\tName\n");
	for (int i = 0; i < processes.length; i++) {
		kprintf("%d\t%s\n", processes.data[i]->pid, processes.data[i]->name);
	}
	for (;;)
		;
}
