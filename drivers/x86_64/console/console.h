#ifndef CONSOLE_H
#define CONSOLE_H

#include "../termios.h"
#include <klibc/resource.h>

#define POLLIN 0x0001
#define POLLOUT 0x0004

#define KBD_BUFFER_SIZE 1024
#define KBD_BIGBUF_SIZE 4096

struct console {
	struct resource res;
	struct termios term;
	size_t width, height;
	bool decckm;
};

extern struct console *console_device;

void add_to_buf(char *ptr, size_t count, bool echo);
void keyboard_init(void);

#endif
