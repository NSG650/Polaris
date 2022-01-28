#ifndef FB_H
#define FB_H

#include <stddef.h>
#include <stdint.h>

struct framebuffer {
	uint8_t *address;
	size_t pitch, bpp;
	uint16_t width, height;
	size_t tex_x, tex_y;
	uint32_t tex_color;
};

void framebuffer_init(struct framebuffer *fb);
void framebuffer_clear(uint32_t color);
void framebuffer_putchar(char c);
void framebuffer_puts(char *string);

#endif
