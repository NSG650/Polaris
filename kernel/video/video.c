/*
 * Copyright 2021 NSG650
 * Copyright 2021 Sebastian
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "video.h"
#include "../klibc/lock.h"
#include "../serial/serial.h"
#define SSFN_CONSOLEBITMAP_TRUECOLOR
#include "ssfn.h"

int cursor_x = 0, cursor_y = 0;

extern unsigned char fb_font;

uint8_t *fb_addr;
size_t fb_pitch, fb_bpp;
uint16_t width_s, height_s;
DECLARE_LOCK(lock);

void vid_reset(void) {
	cursor_x = 0;
	cursor_y = 0;
}

void video_init(struct stivale2_struct_tag_framebuffer *framebuffer) {
	fb_addr = (uint8_t *)framebuffer->framebuffer_addr;
	fb_pitch = framebuffer->framebuffer_pitch;
	fb_bpp = framebuffer->framebuffer_bpp;
	width_s = framebuffer->framebuffer_width;
	height_s = framebuffer->framebuffer_height;

	ssfn_src = (ssfn_font_t *)&fb_font;

	ssfn_dst.ptr = fb_addr;
	ssfn_dst.p = fb_pitch;
	ssfn_dst.w = width_s;
	ssfn_dst.h = height_s;
	ssfn_dst.x = ssfn_dst.y = 0;
}

void draw_px(int x, int y, uint32_t color) {
	size_t py = y * fb_pitch;
	size_t px = x * (fb_bpp / 8);
	*(uint32_t *)(&fb_addr[px + py]) = color;
}

void knewline(void) {
	for (uint32_t y = ssfn_src->height; y != height_s; ++y) {
		void *dest =
			(void *)(((uintptr_t)fb_addr) + (y - ssfn_src->height) * fb_pitch);
		const void *src = (void *)(((uintptr_t)fb_addr) + y * fb_pitch);
		memcpy(dest, src, width_s * 4);
	}

	cursor_y--;
	cursor_x = 131;

	size_t x = (cursor_x * ssfn_src->width) - ssfn_src->width,
		   y = cursor_y * ssfn_src->height;

	size_t i, j;
	while (x >= (2 * ssfn_src->width)) {
		for (i = 0; i < ssfn_src->height; i++) {
			for (j = 0; j < ssfn_src->width; j++) {
				draw_px(x + j, y + i, 0x000000);
			}
		}
		x = (cursor_x * ssfn_src->width) - ssfn_src->width,
		y = cursor_y * ssfn_src->height;
		cursor_x--;
	}
}

void putchar_at(int c, int position_x, int position_y, uint32_t color,
				uint32_t bgcolor) {
	switch (c) {
		case '\n':
			cursor_y++;
			cursor_x = 0;
			return;

		case '\r':
			cursor_x = 0;
			return;

		case '\t':
			cursor_x += 4;
			return;

		case '\b':
			cursor_x--;
			return;
	}

	if ((cursor_y * ssfn_src->height) >= height_s) {
		knewline();
		cursor_x--;
		putchar_at(c, cursor_x, cursor_y, color, bgcolor);
		cursor_x--;
	}

	if (ssfn_dst.fg != color) {
		ssfn_dst.fg = color;
	}

	if (ssfn_dst.bg != bgcolor) {
		ssfn_dst.bg = bgcolor;
	}

	ssfn_dst.x = position_x * ssfn_src->width;
	ssfn_dst.y = position_y * ssfn_src->height;

	ssfn_putc(c);

	if (((cursor_x * ssfn_src->width) + (2 * ssfn_src->width)) >=
		width_s - (2 * ssfn_src->width)) {
		cursor_y++;
		cursor_x = 0;
		return;
	}

	if (c != '\n') {
		cursor_x++;
	}
}

void putchar_color(int c, uint32_t color, uint32_t bgcolor) {
	putchar_at(c, cursor_x, cursor_y, color, bgcolor);
}

void putcharx(int c) {
	putchar_color(c, 0xFFFFFF, 0x000000);
}

void _putchar(char character) {
	putchar_color(character, 0xEEEEEE, 0x000000);
	write_serial_char(character);
}

void kprint_color(char *string, uint32_t color) {
	while (*string) {
		putchar_at(*string++, cursor_x, cursor_y, color, 0x000000);
	}
}

void kprintbgc(char *string, uint32_t fcolor, uint32_t bcolor) {
	LOCK(lock);
	asm volatile("cli");
	while (*string) {
		putchar_color(ssfn_utf8(&string), fcolor, bcolor);
	}
	UNLOCK(lock);
	asm volatile("sti");
}

void kprint(char *string) {
	kprintbgc(string, 0xFFFFFF, 0x000000);
}

void draw_rect(int width, int height, int offx, int offy, uint32_t color) {
	for (int i = 0; i < width + 1; i++) {
		for (int j = 0; j < height + 1; j++) {
			draw_px(i + offx, j + offy, color);
		}
	}
}

void clear_screen(uint32_t color) {
	uint32_t x, y;

	for (x = 0; x < width_s; x++) {
		for (y = 0; y < height_s; y++) {
			draw_px(x, y, color);
		}
	}

	ssfn_dst.bg = color;

	vid_reset();
}
