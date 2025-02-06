#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct syscall_arguments {
	uint64_t syscall_nr;
	uint64_t args0;
	uint64_t args1;
	uint64_t args2;
	uint64_t args3;
	uint64_t args4;
	uint64_t args5;
	uint64_t ret;
};

typedef void (*syscall_handler_t)(struct syscall_arguments *);

extern syscall_handler_t syscalls[];
extern char *syscalls_name[];

void syscall_install_handler(void);
#define syscall_register_handler(A, B) \
	syscalls[A] = B;                   \
	syscalls_name[A] = #B
void syscall_handle(struct syscall_arguments *args);
uint64_t syscall_helper_user_to_kernel_address(uintptr_t user_addr);
bool syscall_helper_copy_to_user(uintptr_t user_addr, void *buffer,
								 size_t count);
bool syscall_helper_copy_from_user(uintptr_t user_addr, void *buffer,
								   size_t count);

#endif
