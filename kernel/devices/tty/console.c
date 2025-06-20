#include <debug/debug.h>
#include <devices/tty/console.h>
#include <devices/tty/termios.h>
#include <errno.h>
#include <fb/fb.h>
#include <klibc/mem.h>
#include <klibc/module.h>
#include <klibc/resource.h>

struct console {
	struct resource res;
	struct termios term;
	size_t width, height;
	bool decckm;
};

struct console *console_device = NULL;

static ssize_t console_write(struct resource *this,
							 struct f_description *description, const void *buf,
							 off_t offset, size_t count) {
	(void)description;
	(void)offset;

	if (!buf) {
		errno = EFAULT;
		return -1;
	}

	spinlock_acquire_or_wait(&this->lock);

	char *r = (char *)buf;
	for (size_t i = 0; i < count; i++) {
		framebuffer_putchar(r[i]);
	}

	spinlock_drop(&this->lock);
	return count;
}

int console_ioctl(struct resource *this, struct f_description *description,
				  uint64_t request, uint64_t arg) {
	(void)description;
	spinlock_acquire_or_wait(&this->lock);

	int ret = 0;

	switch (request) {
		case TCGETS: {
			struct termios *t = (void *)arg;
			if (t)
				*t = console_device->term;
			break;
		}
		case TCSETS:
		case TCSETSW:
		case TCSETSF: {
			struct termios *t = (void *)arg;
			if (t)
				console_device->term = *t;
			break;
		}
		case TIOCGWINSZ: {
			struct winsize *w = (void *)arg;
			if (w) {
				w->ws_row = framebuff.ctx->rows;
				w->ws_col = framebuff.ctx->cols;
				w->ws_xpixel = framebuff.width;
				w->ws_ypixel = framebuff.height;
			} else {
				errno = EINVAL;
				ret = -1;
			}
			break;
		}
		default:
			errno = EINVAL;
			ret = -1;
			break;
	}

	spinlock_drop(&this->lock);
	return ret;
}

static void dec_private(uint64_t esc_val_count, uint32_t *esc_values,
						uint64_t final) {
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
				default:
					break;
			}
	}
}

static void term_callback(struct flanterm_context *term, uint64_t t, uint64_t a,
						  uint64_t b, uint64_t c) {
	(void)term;

	switch (t) {
		case 10:
			dec_private(a, (void *)b, c);
	}
}

void console_init(void) {
	console_device = resource_create(sizeof(struct console));

	console_device->res.stat.st_size = 0;
	console_device->res.stat.st_blocks = 0;
	console_device->res.stat.st_blksize = 4096;
	console_device->res.stat.st_rdev = resource_create_dev_id();
	console_device->res.stat.st_mode = 0644 | S_IFCHR;

	console_device->width = framebuff.width / 8;
	console_device->height = framebuff.height / 16;

	console_device->term.c_iflag = BRKINT | IGNPAR | ICRNL | IXON | IMAXBEL;
	console_device->term.c_oflag = OPOST | ONLCR;
	console_device->term.c_cflag = CS8 | CREAD;
	console_device->term.c_lflag =
		ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE;
	console_device->term.c_cc[VINTR] = CTRL('C');
	console_device->term.c_cc[VEOF] = CTRL('D');
	console_device->term.c_cc[VSUSP] = CTRL('Z');

	console_device->term.ibaud = 38400;
	console_device->term.obaud = 38400;

	console_device->res.status |= POLLOUT;

	console_device->res.write = console_write;
	console_device->res.ioctl = console_ioctl;

	console_device->decckm = false;

	devtmpfs_add_device((struct resource *)console_device, "console");

	kprintffos(false, "Bye bye!\n");
	framebuffer_clear(0x00eee8d5, 0);
	framebuff.ctx->callback = term_callback;
}
