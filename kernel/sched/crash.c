#include <debug/debug.h>
#include <sched/crash.h>
#include <sched/sched.h>
#include <sys/prcb.h>
#include <sys/timer.h>

void sched_display_crash_message(uintptr_t crash_address, struct process *proc,
								 const char *reason) {
	kprintf("\n\nAn user thread crashed under the process %s because of a %s. "
			"It crashed at %p.\n",
			proc->name, reason, crash_address);
	kprintf("You can check the kernel logs for more information about the "
			"crash.\n\n");
}
