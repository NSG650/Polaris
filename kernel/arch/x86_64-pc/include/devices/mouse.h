#ifndef MOUSE_H
#define MOUSE_H

#include <klibc/resource.h>
#include <stddef.h>
#include <stdint.h>

struct mouse_packet {
	uint8_t flags;
	uint8_t x_mov;
	uint8_t y_mov;
};

struct mouse_device {
	struct resource res;
	struct mouse_packet *pack;
	bool available;
};

void mouse_init(void);

#endif
