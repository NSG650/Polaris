#include <debug/debug.h>
#include <errno.h>
#include <sched/syscall.h>
#include <sys/prcb.h>

syscall_handler_t syscalls[512] = {NULL};

void syscall_handle(struct syscall_arguments *args) {
	if (args->syscall_nr > 512 || syscalls[args->syscall_nr] == NULL) {
		args->ret = -1;
		errno = ENOSYS;
		return;
	}
	syscalls[args->syscall_nr](args);
}
