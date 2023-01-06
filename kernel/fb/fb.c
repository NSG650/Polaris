#include "debug/debug.h"
#include "terminal/backends/framebuffer.h"
#include "terminal/term.h"
#include <fb/fb.h>
#include <klibc/mem.h>
#include <locks/spinlock.h>
#include <mm/slab.h>

/*
 * Copyright 2021, 2022 NSG650
 * Copyright 2021, 2022 Sebastian
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

uint8_t framebuffer_initialised = 0;

struct framebuffer framebuff;
static struct term_context *ctx;

void framebuffer_init(struct framebuffer *fb) {
	framebuff.address = fb->address;
	framebuff.pitch = fb->pitch;
	framebuff.bpp = fb->bpp;
	framebuff.width = fb->width;
	framebuff.height = fb->height;
	framebuff.tex_x = fb->tex_x;
	framebuff.tex_y = fb->tex_y;
	framebuff.tex_color = fb->tex_color;
	framebuff.tex_height = fb->height / 16;
	framebuff.tex_width = fb->width / 8;
	framebuff.back_address = kmalloc(framebuff.pitch * framebuff.height);
	framebuff.bg_color = fb->bg_color;

	ctx = fbterm_init(kmalloc, (void *)fb->address, fb->width, fb->height,
					  fb->pitch, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
					  0, 0, 0, 1, 1, 0);

	framebuffer_clear(fb->tex_color, fb->bg_color);

	framebuffer_initialised = 1;
}

static void ultoa_strcat(char *dest, uint64_t src, bool add_semicolon) {
	char buf[65];
	strcat(dest, ultoa(src, buf, 10));
	if (add_semicolon == true) {
		strcat(dest, ";");
	}
}

// Format: 0xAARRGGBB, alpha is ignored
void framebuffer_clear(uint32_t foreground, uint32_t background) {
	uint8_t red_fg = (foreground & 0xFF0000) >> 16;
	uint8_t green_fg = (foreground & 0xFF00) >> 8;
	uint8_t blue_fg = foreground & 0xFF;

	uint8_t red_bg = (background & 0xFF0000) >> 16;
	uint8_t green_bg = (background & 0xFF00) >> 8;
	uint8_t blue_bg = background & 0xFF;

	char result[128] = "\033[38;2;";

	ultoa_strcat(result, red_fg, true);
	ultoa_strcat(result, green_fg, true);
	ultoa_strcat(result, blue_fg, true);

	strcat(result, "48;2;");

	ultoa_strcat(result, red_bg, true);
	ultoa_strcat(result, green_bg, true);
	ultoa_strcat(result, blue_bg, false);

	strcat(result, "m");

	term_write(ctx, result, strlen(result));
	term_write(ctx, "\033[2J", 5);
	term_write(ctx, "\033[H", 4);
}

void framebuffer_putchar(char c) {
	term_write(ctx, &c, 1);
}

void framebuffer_puts(char *string) {
	term_write(ctx, string, strlen(string));
}
