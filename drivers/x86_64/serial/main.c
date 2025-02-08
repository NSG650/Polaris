#include "../termios.h"
#include <debug/debug.h>
#include <errno.h>
#include <fs/devtmpfs.h>
#include <klibc/mem.h>
#include <klibc/module.h>
#include <klibc/resource.h>
#include <stddef.h>
#include <stdint.h>

struct serial_tty_info {
	uint16_t width, height;
	size_t tex_x, tex_y;
	struct termios termios_info;
};

struct serial_resource {
	struct resource res;
	struct serial_tty_info info;
};

struct serial_resource *ser = NULL;

void driver_exit(void) {
	kprintf("Bye bye!\n");
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
		if ((ser->res.status & POLLIN) != 0) {
			ser->res.status &= ~POLLIN;
			event_trigger(&ser->res.event, false);
		}
		if (c == '\r') {
			serial_putchar('\n');
			break;
		}
		if (c == 0x7f) {
			if (bytes_read != 0) {
				serial_putchar('\b');
				serial_putchar(' ');
				serial_putchar('\b');
				buf[bytes_read--] = 0;
			}
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
	int ret = 0;
	switch (request) {
		case TCGETS: {
			struct termios *t = (struct termios *)arg;
			*t = ser->info.termios_info;
			break;
		}
		case TCSETS:
		case TCSETSW:
		case TCSETSF: {
			struct termios *t = (struct termios *)arg;
			ser->info.termios_info = *t;
			break;
		}
		case TIOCGWINSZ: {
			struct winsize *w = (struct winsize *)arg;
			w->ws_row = 80;
			w->ws_col = 25;
			w->ws_xpixel = 80 * 8;
			w->ws_ypixel = 25 * 16;
			break;
		}
		default: {
			errno = EINVAL;
			ret = -1;
			break;
		}
	}
	spinlock_drop(&this->lock);
	return ret;
}

static ssize_t serial_write(struct resource *_this,
							struct f_description *description, const void *_buf,
							off_t offset, size_t count) {
	spinlock_acquire_or_wait(&_this->lock);
	char *buf = (char *)_buf;

	for (size_t i = 0; i < count; i++) {
		serial_putchar(buf[i]);
	}

	spinlock_drop(&_this->lock);

	return count;
}

uint64_t driver_entry(struct module *driver_module) {
	driver_module->exit = driver_exit;

	ser = resource_create(sizeof(struct serial_resource));

	ser->res.read = serial_read;
	ser->res.write = serial_write;
	ser->res.ioctl = serial_ioctl;
	ser->res.stat.st_size = 0;
	ser->res.stat.st_blocks = 0;
	ser->res.stat.st_blksize = 4096;
	ser->res.stat.st_rdev = resource_create_dev_id();
	ser->res.stat.st_mode = 0644 | S_IFCHR;
	ser->info.height = 80 * 8;
	ser->info.width = 25 * 16;
	ser->info.tex_x = 0;
	ser->info.tex_y = 0;

	ser->info.termios_info.c_iflag = IGNPAR | ICRNL | IXON | IMAXBEL;
	ser->info.termios_info.c_oflag = OPOST | ONLCR;
	ser->info.termios_info.c_cflag = CS8 | CREAD;
	ser->info.termios_info.c_lflag =
		ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE;

	ser->res.status |= POLLOUT;

	devtmpfs_add_device((struct resource *)ser, "stty");

	return 0;
}
