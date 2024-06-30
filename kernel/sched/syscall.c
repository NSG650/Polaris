#include <debug/debug.h>
#include <errno.h>
#include <sched/syscall.h>
#include <sys/prcb.h>

syscall_handler_t syscalls[512] = {NULL};
char *syscalls_name[512] = {NULL};

void syscall_handle(struct syscall_arguments *args) {
	if (args->syscall_nr > 512 || syscalls[args->syscall_nr] == NULL) {
		args->ret = -1;
		errno = ENOSYS;
		return;
	}
	syscalls[args->syscall_nr](args);
#if 0
	if ((int)args->ret < 0) {
		kprintf("%s failed. Called by process %s with errno %d\n", syscalls_name[args->syscall_nr], prcb_return_current_cpu()->running_thread->mother_proc->name, errno);
	}
#endif
}
