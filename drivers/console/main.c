#include "termios.h"
#include <debug/debug.h>
#include <fb/fb.h>
#include <fs/devtmpfs.h>
#include <klibc/mem.h>
#include <klibc/module.h>

struct framebuffer *framebuff = NULL;

struct console_info {
	uint16_t width, height;
	size_t tex_x, tex_y;
	uint32_t tex_color;
	uint32_t bg_color;
	struct termios termios;
};

struct console {
	struct resource res;
	struct console_info info;
	bool decckm;
};

struct console *console_device;

static ssize_t console_read(struct resource *_this,
							struct f_description *description, void *_buf,
							off_t offset, size_t count) {
	(void)description;
	(void)offset;
	(void)_buf;
	return -1;
}

int console_ioctl(struct resource *this, struct f_description *description,
				  uint64_t request, uint64_t arg) {
	spinlock_acquire_or_wait(&this->lock);

	if (!arg) {
		return -1;
	}

	switch (request) {
		case TCGETS: {
			struct termios *t = (void *)arg;
			if (t)
				*t = console_device->info.termios;
			spinlock_drop(&this->lock);
			return 0;
		}
		case TCSETS:
		case TCSETSW:
		case TCSETSF: {
			struct termios *t = (void *)arg;
			if (t)
				console_device->info.termios = *t;
			spinlock_drop(&this->lock);
			return 0;
		}
		case TIOCGWINSZ: {
			struct winsize *w = (void *)arg;
			if (!w) {
				spinlock_drop(&this->lock);
				return -1;
			}
			w->ws_row = console_device->info.width;
			w->ws_col = console_device->info.height;
			w->ws_xpixel = framebuff->width;
			w->ws_ypixel = framebuff->height;
			spinlock_drop(&this->lock);
			return 0;
		}
		default:
			spinlock_drop(&this->lock);
			return resource_default_ioctl(this, description, request, arg);
	}
}

static ssize_t console_write(struct resource *_this,
							 struct f_description *description, const void *buf,
							 off_t offset, size_t count) {
	(void)description;
	(void)offset;
	if (!buf) {
		return -1;
	}
	spinlock_acquire_or_wait(&_this->lock);
	char *r = (char *)buf;
	if (!r) {
		spinlock_drop(&_this->lock);
		return -1;
	}
	for (size_t i = 0; i < count; i++) {
		framebuffer_putchar(r[i]);
	}
	spinlock_drop(&_this->lock);
	return count;
}

uint64_t driver_entry(struct module *driver_module) {
	strncpy(driver_module->name, "console", sizeof(driver_module->name));

	framebuff = framebuffer_info();
	if (!framebuff) {
		return 2;
	}

	framebuffer_clear(0x00eee8d5, 0);
	kprintffos(0, "Bye bye!\n");

	console_device = resource_create(sizeof(struct console));

	console_device->res.stat.st_size = 0;
	console_device->res.stat.st_blocks = 0;
	console_device->res.stat.st_blksize = 4096;
	console_device->res.stat.st_rdev = resource_create_dev_id();
	console_device->res.stat.st_mode = 0644 | S_IFCHR;

	console_device->info.height = framebuff->height / 16;
	console_device->info.width = framebuff->width / 8;
	console_device->info.tex_x = 0;
	console_device->info.tex_y = 0;

	console_device->info.termios.c_iflag =
		BRKINT | IGNPAR | ICRNL | IXON | IMAXBEL;
	console_device->info.termios.c_oflag = OPOST | ONLCR;
	console_device->info.termios.c_cflag = CS8 | CREAD;
	console_device->info.termios.c_lflag =
		ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE;

	console_device->info.termios.c_cc[VINTR] = CTRL('C');
	console_device->info.termios.c_cc[VEOF] = CTRL('D');
	console_device->info.termios.c_cc[VSUSP] = CTRL('Z');

	console_device->info.termios.ibaud = 38400;
	console_device->info.termios.obaud = 38400;

	console_device->res.read = console_read;
	console_device->res.write = console_write;
	console_device->res.ioctl = console_ioctl;

	devtmpfs_add_device((struct resource *)console_device, "console");

	return 0;
}
