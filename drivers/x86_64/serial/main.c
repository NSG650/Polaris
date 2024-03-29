#include "serial.h"
#include <debug/debug.h>
#include <klibc/mem.h>
#include <klibc/module.h>
#include <serial/serial.h>

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

	switch (request) {
		case TCGETS: {
			struct termios *t = (struct termios *)arg;
			*t = ser->info.termios_info;
			spinlock_drop(&this->lock);
			return 0;
		}
		case TCSETS:
		case TCSETSW:
		case TCSETSF: {
			struct termios *t = (struct termios *)arg;
			ser->info.termios_info = *t;
			spinlock_drop(&this->lock);
			return 0;
		}
		case TIOCGWINSZ: {
			struct winsize *w = (struct winsize *)arg;
			w->ws_row = 80;
			w->ws_col = 25;
			w->ws_xpixel = 80 * 8;
			w->ws_ypixel = 25 * 16;
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

	devtmpfs_add_device((struct resource *)ser, "stty");

	return 0;
}
