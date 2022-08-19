#include <fs/devtmpfs.h>
#include <fb/fb.h>
#include <io/ports.h>
#include <klibc/mem.h>
#include <mm/slab.h>
#include <debug/debug.h>
#include <devices/keyboard.h>

struct console_info {
	uint16_t width, height;
	size_t tex_x, tex_y;
	uint32_t tex_color;
	uint32_t bg_color;
};

struct console {
	struct resource;
	char **console_buffer;
	struct console_info info;
};

struct console *console_device;

static bool is_printable(char c) {
    return (c >= 0x20 && c <= 0x7e);
}

static ssize_t console_read(struct resource *_this, struct f_description *description, void *_buf, off_t offset, size_t count) {
	char *a = (char*)_buf;
	keyboard_gets(a, count);
	return 0;
}

static ssize_t console_write(struct resource *_this, struct f_description *description, const void *buf, off_t offset, size_t count) {
	char *r = (char *)buf;
	for (size_t i = 0; i < count; i++) {
		framebuffer_putchar(r[i]);
	}
	return 0;
}
/*
static int console_ioctl(struct resource *_this, struct f_description *description, uint64_t request, uint64_t argp) {
	struct console *con = (struct console *)_this;
	return -1;
}
*/
void console_init(void) {
	framebuffer_clear(0);
	kprintffos(0, "No more kprints on the framebuffer *vine boom*\n");
	console_device = resource_create(sizeof(struct console));
	console_device->read = console_read;
	console_device->write = console_write;
	console_device->ioctl = resource_default_ioctl;
	console_device->stat.st_size = 0;
	console_device->stat.st_blocks = 0;
	console_device->stat.st_blksize = 4096;
	console_device->stat.st_rdev = resource_create_dev_id();
	console_device->stat.st_mode = 0644 | S_IFCHR;
	console_device->info.height = framebuff.height / 16;
	console_device->info.width = framebuff.width / 8;
	console_device->info.tex_x = 0;
	console_device->info.tex_y = 0;
	console_device->console_buffer = kmalloc(console_device->info.height * console_device->info.width);
	devtmpfs_add_device((struct resource *)console_device, "console");
	keyboard_init();
}
