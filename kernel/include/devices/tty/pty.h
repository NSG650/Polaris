#ifndef PTY_H
#define PTY_H

#include <devices/tty/termios.h>
#include <klibc/resource.h>
#include <sched/syscall.h>
#include <stddef.h>
#include <stdint.h>

struct ring_buffer {
	uint8_t *data;
	size_t data_length;
	uintptr_t read_ptr;
	uintptr_t write_ptr;
	size_t used;
	struct event ev;
};

struct pty {
	int name;
	struct pty_slave *ps;
	struct pty_master *pm;
	struct termios term;
	struct winsize ws;
	struct ring_buffer in;
	struct ring_buffer out;
};

struct pty_slave {
	struct resource res;
	struct pty *pty;
};

struct pty_master {
	struct resource res;
	struct pty *pty;
};

void syscall_openpty(struct syscall_arguments *args);

#endif