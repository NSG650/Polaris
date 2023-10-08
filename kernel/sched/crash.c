#include <debug/debug.h>
#include <fb/fb.h>
#include <sched/crash.h>
#include <sched/sched.h>
#include <sys/prcb.h>
#include <sys/timer.h>

void sched_display_crash_message(uintptr_t crash_address, struct process *proc,
								 const char *reason) {
	if (!framebuff.address)
		return;

	char *crash_message = kmalloc(4096);
	strcpy(crash_message, "\n\nAn user thread crashed under the process ");
	strcat(crash_message, proc->name);
	strcat(crash_message, " because of a ");
	strcat(crash_message, reason);
	strcat(crash_message, ". It crashed at 0x");
	char *convert_me_to_hex = kmalloc(21);
	ultoa(crash_address, convert_me_to_hex, 16);
	strcat(crash_message, convert_me_to_hex);
	strcat(crash_message, ".\n");
	framebuffer_puts(crash_message);
	framebuffer_puts("You can check the kernel logs for more information about "
					 "the crash.\n\n");

	kfree(crash_message);
	kfree(convert_me_to_hex);
}
