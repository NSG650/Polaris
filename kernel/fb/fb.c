#include "terminal/backends/fb.h"
#include "debug/debug.h"
#include <fb/fb.h>
#include <klibc/mem.h>
#include <locks/spinlock.h>
#include <mm/slab.h>

/*
 * Copyright 2021 - 2023 NSG650
 * Copyright 2021 - 2023 Neptune
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
static struct flanterm_context *ctx;

static void kffree(void *addr, size_t sz) {
	(void)sz;
	kfree(addr);
}

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

	framebuff.color_masks[0] = fb->color_masks[0];
	framebuff.color_masks[1] = fb->color_masks[1];
	framebuff.color_masks[2] = fb->color_masks[2];

	ctx = flanterm_fb_init(kmalloc, kffree, (void *)fb->address, fb->width,
						   fb->height, fb->pitch, fb->color_masks[0].length,
						   fb->color_masks[0].offset, fb->color_masks[1].length,
						   fb->color_masks[1].offset, fb->color_masks[2].length,
						   fb->color_masks[2].offset, NULL, NULL, NULL, NULL,
						   NULL, NULL, NULL, NULL, 0, 0, 1, 1, 1, 0);

	framebuff.ctx = ctx;

	framebuffer_clear(fb->tex_color, fb->bg_color);

	framebuffer_initialised = 1;
}

void framebuffer_set_callback(void (*callback)(struct flanterm_context *,
											   uint64_t, uint64_t, uint64_t,
											   uint64_t)) {
	if (ctx) {
		ctx->callback = callback;
	}
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

	flanterm_write(ctx, result, strlen(result));
	flanterm_write(ctx, "\033[2J", 5);
	flanterm_write(ctx, "\033[H", 4);
}

void framebuffer_putchar(char c) {
	flanterm_write(ctx, &c, 1);
}

void framebuffer_puts(char *string) {
	flanterm_write(ctx, string, strlen(string));
}

struct framebuffer *framebuffer_info(void) {
	return &framebuff;
}
