#include "termios.h"
#include <debug/debug.h>
#include <devices/keyboard.h>
#include <errno.h>
#include <fb/fb.h>
#include <fs/devtmpfs.h>
#include <io/ports.h>
#include <klibc/mem.h>
#include <mm/slab.h>

struct console_info {
	uint16_t width, height;
	size_t tex_x, tex_y;
	uint32_t tex_color;
	uint32_t bg_color;
	struct termios termios_info;
};

struct console {
	struct resource res;
	struct console_info info;
};

struct console *console_device;

static ssize_t console_read(struct resource *_this,
							struct f_description *description, void *_buf,
							off_t offset, size_t count) {
	(void)_this;
	(void)description;
	(void)offset;
	char *a = (char *)_buf;

	if (count == 1) { // for micropython AAAAAAAAA
		keyboard_gets(a, count, 0);
		return count;
	}
	size_t c = keyboard_gets(a, count, 1);

	return (ssize_t)c;
}

int console_ioctl(struct resource *this, struct f_description *description,
				  uint64_t request, uint64_t arg) {

	if (!arg)
		return -1;

	switch (request) {
		case TCGETS: {
			struct termios *t = (void *)arg;
			*t = console_device->info.termios_info;
			return 0;
		}
		case TCSETS:
		case TCSETSW:
		case TCSETSF: {
			struct termios *t = (void *)arg;
			console_device->info.termios_info = *t;
			return 0;
		}
		case TIOCGWINSZ: {
			struct winsize *w = (void *)arg;
			w->ws_row = console_device->info.width;
			w->ws_col = console_device->info.height;
			w->ws_xpixel = framebuff.width;
			w->ws_ypixel = framebuff.height;
			return 0;
		}
		default:
			return resource_default_ioctl(this, description, request, arg);
	}
}

static ssize_t console_write(struct resource *_this,
							 struct f_description *description, const void *buf,
							 off_t offset, size_t count) {
	(void)_this;
	(void)description;
	(void)offset;
	char *r = (char *)buf;
	for (size_t i = 0; i < count; i++) {
		framebuffer_putchar(r[i]);
	}
	return count;
}

void console_init(void) {
	framebuffer_clear(0x00eee8d5, 0);
	kprintffos(0, "Bye bye!\n");
	console_device = resource_create(sizeof(struct console));
	console_device->res.read = console_read;
	console_device->res.write = console_write;
	console_device->res.ioctl = console_ioctl;
	console_device->res.stat.st_size = 0;
	console_device->res.stat.st_blocks = 0;
	console_device->res.stat.st_blksize = 4096;
	console_device->res.stat.st_rdev = resource_create_dev_id();
	console_device->res.stat.st_mode = 0644 | S_IFCHR;
	console_device->info.height = framebuff.height / 16;
	console_device->info.width = framebuff.width / 8;
	console_device->info.tex_x = 0;
	console_device->info.tex_y = 0;

	console_device->info.termios_info.c_iflag = IGNPAR | ICRNL | IXON | IMAXBEL;
	console_device->info.termios_info.c_oflag = OPOST | ONLCR;
	console_device->info.termios_info.c_cflag = CS8 | CREAD;
	console_device->info.termios_info.c_lflag =
		ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE;

	devtmpfs_add_device((struct resource *)console_device, "console");
	keyboard_init();
}
