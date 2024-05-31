#ifndef KARGS_H
#define KARGS_H

#include <klibc/mem.h>
#include <stddef.h>
#include <stdint.h>

enum kernel_args_enum_t {
	KERNEL_ARGS_NO_LAI = 1,
	KERNEL_ARGS_CPU_COUNT_GIVEN = 1 << 1,
	KERNEL_ARGS_KPRINTF_LOGS = 1 << 2,
	KERNEL_ARGS_INIT_PATH_GIVEN = 1 << 3,
	KERNEL_ARGS_SUPPRESS_UBSAN = 1 << 4,
	KERNEL_ARGS_ALLOW_WRITES_TO_DISKS = 1 << 5,
	KERNEL_ARGS_SUPPRESS_USER_DEBUG_MESSAGES = 1 << 6,
};

struct kernel_args {
	uint16_t kernel_args;
	uint32_t cpu_count;
	char *init_binary_path;
};

extern struct kernel_args kernel_arguments;

void kargs_init(char *args);

#endif
