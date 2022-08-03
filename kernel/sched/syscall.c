#include <errno.h>
#include <sched/syscall.h>
#include <sys/prcb.h>

syscall_handler_t syscalls[256] = {NULL};

void syscall_handle(struct syscall_arguments *args) {
	if (syscalls[args->syscall_nr] == NULL) {
		prcb_return_current_cpu()->running_thread->errno = -ENOSYS;
		return;
	}
	syscalls[args->syscall_nr](args);
}
