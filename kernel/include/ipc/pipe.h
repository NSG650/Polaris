#ifndef PIPE_H
#define PIPE_H

#include <fs/vfs.h>
#include <sched/sched.h>
#include <stddef.h>
#include <stdint.h>

struct pipe {
	struct resource res;
	void *data;
	size_t data_length;
	uintptr_t read_ptr;
	uintptr_t write_ptr;
};

void syscall_pipe(struct syscall_arguments *args);

#endif