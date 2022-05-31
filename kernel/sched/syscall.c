#include <sched/syscall.h>

syscall_handler_t syscalls[256] = {NULL};

void syscall_register_handler(int n, void *handler) {
	syscalls[n] = handler;
}

void syscall_handle(struct syscall_arguments *args) {
	if (syscalls[args->syscall_nr] == NULL) {
		args->syscall_nr = 0x41b;
		return;
	}
	syscalls[args->syscall_nr](args);
}
