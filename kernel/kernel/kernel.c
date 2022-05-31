#include <debug/debug.h>
#include <kernel.h>
#include <sched/sched.h>
#include <sys/prcb.h>

void kernel_main(void *args) {
	uint64_t *module_info = (uint64_t*)args;
	kprintf("ELF binary located at 0x%p\n", module_info[0]);
	kprintf("Hello I am %s\n",
			prcb_return_current_cpu()->running_thread->mother_proc->name);
	kprintf("Creating elf user process\n");
	process_create_elf("init", 0, 2000, (uint8_t*)module_info[0]);
	for (;;)
		;
}
