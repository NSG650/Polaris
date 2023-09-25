#ifndef FB_H
#define FB_H

#include <stddef.h>
#include <stdint.h>

struct framebuffer {
	uint32_t *address;
	uint32_t *back_address;
	size_t pitch, bpp;
	uint16_t width, height;
	size_t tex_x, tex_y;
	uint16_t tex_width, tex_height;
	uint32_t tex_color;
	uint32_t bg_color;
};

void framebuffer_clear(uint32_t foreground, uint32_t background);
void framebuffer_putchar(char c);
void framebuffer_puts(char *string);
struct framebuffer *framebuffer_info(void);

#endif
