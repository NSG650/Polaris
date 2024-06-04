#ifndef SERIAL_TTY_H
#define SERIAL_TTY_H

#include "termios.h"
#include <fs/devtmpfs.h>
#include <klibc/resource.h>
#include <stddef.h>
#include <stdint.h>

struct serial_tty_info {
	uint16_t width, height;
	size_t tex_x, tex_y;
	struct termios termios_info;
};

struct serial_resource {
	struct resource res;
	struct serial_tty_info info;
};

#endif
