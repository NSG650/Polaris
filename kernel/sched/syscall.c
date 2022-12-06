#include <errno.h>
#include <sched/syscall.h>
#include <sys/prcb.h>
#include <debug/debug.h>

syscall_handler_t syscalls[512] = {NULL};

void syscall_handle(struct syscall_arguments *args) {
	if (syscalls[args->syscall_nr] == NULL) {
		errno = -ENOSYS;
		return;
	}
	kprintf("syscall #%d: %d, %d, %d, %d\n", args->syscall_nr, args->args0, args->args1, args->args2, args->args3);
	syscalls[args->syscall_nr](args);
}
