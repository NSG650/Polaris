#include "../console/termios.h"
#include <debug/debug.h>
#include <fb/fb.h>
#include <fs/devtmpfs.h>
#include <klibc/mem.h>
#include <klibc/module.h>
#include <x86_64/io/ports.h>

#define COM2 0x2F8

struct serial_tty_info {
	uint16_t width, height;
	size_t tex_x, tex_y;
	uint32_t tex_color;
	uint32_t bg_color;
	struct termios termios_info;
};

struct serial_tty {
	struct resource res;
	struct serial_tty_info info;
};

struct serial_tty *serial_device = NULL;

static inline bool is_transmit_empty(void) {
	return (inb(COM2 + 5) & 0b1000000) != 0;
}

static inline void transmit_data(uint8_t value) {
	while (!is_transmit_empty()) {
		asm volatile("pause");
	}

	outb(COM2, value);
}

static void serial_putchar(char ch) {
	transmit_data(ch);
}

static void serial_puts(char *str) {
	while (*str) {
		if (*str == '\n')
			transmit_data('\r');
		transmit_data(*str++);
	}
}

static int serial_received(void) {
	return inb(COM2 + 5) & 1;
}

static char serial_get_byte(void) {
	while (serial_received() == 0) {
		asm volatile("pause");
	}

	return inb(COM2);
}

static ssize_t serial_read(struct resource *_this,
						   struct f_description *description, void *_buf,
						   off_t offset, size_t count) {
	spinlock_acquire_or_wait(&_this->lock);
	char c = '\0';
	ssize_t bytes_read = 0;
	char *buf = (char *)_buf;
	while (bytes_read < count) {
		c = serial_get_byte();
		if (c == '\r') {
			serial_putchar('\r');
			serial_putchar('\n');
			break;
		}
		serial_putchar(c);
		buf[bytes_read++] = c;
	}
	spinlock_drop(&_this->lock);
	return bytes_read;
}

int serial_ioctl(struct resource *this, struct f_description *description,
				 uint64_t request, uint64_t arg) {
	spinlock_acquire_or_wait(&this->lock);

	if (!arg) {
		return -1;
	}

	switch (request) {
		case TCGETS: {
			struct termios *t = (void *)arg;
			*t = serial_device->info.termios_info;
			spinlock_drop(&this->lock);
			return 0;
		}
		case TCSETS:
		case TCSETSW:
		case TCSETSF: {
			struct termios *t = (void *)arg;
			serial_device->info.termios_info = *t;
			spinlock_drop(&this->lock);
			return 0;
		}
		case TIOCGWINSZ: {
			struct winsize *w = (void *)arg;
			w->ws_row = 100;
			w->ws_col = 30;
			w->ws_xpixel = 100 * 8;
			w->ws_ypixel = 30 * 16;
			spinlock_drop(&this->lock);
			return 0;
		}
		default:
			spinlock_drop(&this->lock);
			return resource_default_ioctl(this, description, request, arg);
	}
}

static ssize_t serial_write(struct resource *_this,
							struct f_description *description, const void *_buf,
							off_t offset, size_t count) {
	spinlock_acquire_or_wait(&_this->lock);
	char *buf = (char *)_buf;

	for (size_t i = 0; i < count; i++) {
		serial_putchar(*buf);
		if (*buf++ == '\n')
			serial_putchar('\r');
	}

	spinlock_drop(&_this->lock);

	return count;
}

uint64_t driver_entry(struct module *driver_module) {
	outb(COM2 + 1, 0x1);
	outb(COM2 + 3, 0x80);
	outb(COM2, 0x1);
	outb(COM2 + 1, 0x0);
	outb(COM2 + 3, 0x3);
	outb(COM2 + 2, 0xC7);
	outb(COM2 + 4, 0xB);

	serial_device = resource_create(sizeof(struct serial_tty));
	serial_device->res.read = serial_read;
	serial_device->res.write = serial_write;
	serial_device->res.ioctl = serial_ioctl;
	serial_device->res.stat.st_size = 0;
	serial_device->res.stat.st_blocks = 0;
	serial_device->res.stat.st_blksize = 4096;
	serial_device->res.stat.st_rdev = resource_create_dev_id();
	serial_device->res.stat.st_mode = 0644 | S_IFCHR;
	serial_device->info.height = 100 * 8;
	serial_device->info.width = 30 * 16;
	serial_device->info.tex_x = 0;
	serial_device->info.tex_y = 0;

	serial_device->info.termios_info.c_iflag = IGNPAR | ICRNL | IXON | IMAXBEL;
	serial_device->info.termios_info.c_oflag = OPOST | ONLCR;
	serial_device->info.termios_info.c_cflag = CS8 | CREAD;
	serial_device->info.termios_info.c_lflag =
		ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE;

	devtmpfs_add_device((struct resource *)serial_device, "stty");

	return 0;
}
