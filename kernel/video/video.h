#ifndef VIDEO_H
#define VIDEO_H

#include "../klibc/mem.h"
#include "../klibc/string.h"
#include <stddef.h>
#include <stdint.h>
#include <stivale2.h>

void vid_reset(void);
void video_init(struct stivale2_struct_tag_framebuffer *framebuffer);
void putchar_color(int c, uint32_t color, uint32_t bgcolor);
void putcharx(int c);
void kprint_color(char *string, uint32_t color);
void kprint(char *string);
void kprintbgc(char *string, uint32_t fcolor, uint32_t bcolor);
void draw_rect(int x, int y, int offx, int offy, uint32_t color);
void clear_screen(uint32_t color);

#endif
