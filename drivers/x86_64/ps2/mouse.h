#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

struct mouse_packet {
	uint8_t flags;
	int32_t delta_x;
	int32_t delta_y;
};

void mouse_init(void);

#endif
