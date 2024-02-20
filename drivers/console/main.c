#include "keyboard.h"
#include "termios.h"
#include <asm/asm.h>
#include <debug/debug.h>
#include <fb/fb.h>
#include <fs/devtmpfs.h>
#include <io/ports.h>
#include <klibc/mem.h>
#include <klibc/module.h>
#include <sys/apic.h>
#include <sys/isr.h>

#define POLLIN 0x0001
#define POLLOUT 0x0004

lock_t term_lock = {0};

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

void *memset(void *b, int c, size_t len) {
	size_t i = 0;
	unsigned char *p = b;
	while (len > 0) {
		*p = c;
		p++;
		len--;
	}
	return b;
}

static bool is_printable(uint8_t c) {
	return c >= 0x20 && c <= 0x7e;
}

static int toupper(int c) {
	if (c >= 'a' && c <= 'z') {
		c -= 0x20;
	}
	return c;
}

struct console *console_device = NULL;

static ssize_t console_read(struct resource *this,
							struct f_description *description, void *_buf,
							off_t offset, size_t count) {
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
			w->ws_xpixel = framebuff.width;
			w->ws_ypixel = framebuff.height;
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
	(void)_this;

	if (!buf) {
		return -1;
	}

	spinlock_acquire_or_wait(&term_lock);

	char *r = (char *)buf;
	for (size_t i = 0; i < count; i++) {
		framebuffer_putchar(r[i]);
	}

	spinlock_drop(&term_lock);
	return count;
}

void dec_private(uint64_t esc_val_count, uint32_t *esc_values, uint64_t final) {
	(void)esc_val_count;

	switch (esc_values[0]) {
		case 1:
			switch (final) {
				case 'h':
					console_device->decckm = true;
					break;
				case 'l':
					console_device->decckm = false;
					break;
			}
	}
}

void term_callback(struct flanterm_context *term, uint64_t t, uint64_t a,
				   uint64_t b, uint64_t c) {
	(void)term;

	switch (t) {
		case 10:
			dec_private(a, (void *)b, c);
	}
}

uint64_t driver_entry(struct module *driver_module) {
	framebuffer_set_callback(term_callback);

	framebuffer_clear(0x00eee8d5, 0);
	kprintffos(0, "Bye bye!\n");

	console_device = resource_create(sizeof(struct console));

	console_device->res.stat.st_size = 0;
	console_device->res.stat.st_blocks = 0;
	console_device->res.stat.st_blksize = 4096;
	console_device->res.stat.st_rdev = resource_create_dev_id();
	console_device->res.stat.st_mode = 0644 | S_IFCHR;

	console_device->info.height = framebuff.height / 16;
	console_device->info.width = framebuff.width / 8;
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

	console_device->res.status |= POLLOUT;

	console_device->res.read = console_read;
	console_device->res.write = console_write;
	console_device->res.ioctl = console_ioctl;

	devtmpfs_add_device((struct resource *)console_device, "console");

	return 0;
}
