#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include <stddef.h>

struct syscall_arguments {
	uint64_t syscall_nr;
	uint64_t args0;
	uint64_t args1;
	uint64_t args2;
	uint64_t args3;
};

typedef void (*syscall_handler_t)(struct syscall_arguments *);

void syscall_install_handler(void);
void syscall_register_handler(int n, void* handler);
void syscall_handle(struct syscall_arguments *args);

#endif
