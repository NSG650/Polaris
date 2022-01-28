#include <asm/asm.h>
#include <debug/debug.h>
#include <fb/fb.h>
#include <serial/serial.h>
#include <stddef.h>
#include <stdint.h>
#include <stivale2.h>

static uint8_t stack[32768];
static struct stivale2_header_tag_smp smp_hdr_tag = {
	.tag = {.identifier = STIVALE2_HEADER_TAG_SMP_ID, .next = 0}, .flags = 1};

static struct stivale2_header_tag_framebuffer framebuffer_hdr_tag = {
	// All tags need to begin with an identifier and a pointer to the next tag
	.tag = {.identifier = STIVALE2_HEADER_TAG_FRAMEBUFFER_ID,
			.next = (uintptr_t)&smp_hdr_tag},
	.framebuffer_width = 0,
	.framebuffer_height = 0,
	.framebuffer_bpp = 0};

__attribute__((section(".stivale2hdr"),
			   used)) static struct stivale2_header stivale_hdr = {
	.entry_point = 0,
	.stack = (uintptr_t)stack + sizeof(stack),
	.flags = (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4),
	.tags = (uintptr_t)&framebuffer_hdr_tag};

void *stivale2_get_tag(struct stivale2_struct *stivale2_struct, uint64_t id) {
	struct stivale2_tag *current_tag = (void *)stivale2_struct->tags;
	for (;;) {
		if (current_tag == NULL) {
			return NULL;
		}

		if (current_tag->identifier == id) {
			return current_tag;
		}

		current_tag = (void *)current_tag->next;
	}
}

void arch_entry(struct stivale2_struct *stivale2_struct) {
	serial_puts("Hello World\n");
	serial_puts("This is another line\n");
	struct framebuffer fb;
	struct stivale2_struct_tag_framebuffer *fb_tag =
		stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_FRAMEBUFFER_ID);
	fb.address = (uint8_t *)fb_tag->framebuffer_addr;
	fb.pitch = fb_tag->framebuffer_pitch;
	fb.bpp = fb_tag->framebuffer_bpp;
	fb.width = fb_tag->framebuffer_width;
	fb.height = fb_tag->framebuffer_height;
	fb.tex_color = 0xffffff;
	fb.tex_x = 0;
	fb.tex_y = 0;
	framebuffer_init(&fb);
	framebuffer_puts("Hello World\n");
	framebuffer_puts("This is another line\n");
	kprintf("printf test: %s\n", "funny");
	kprintf("location of kputs: %x\n", kputs);
	for (;;) {
		cli();
		halt();
	}
}
