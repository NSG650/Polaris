#include "console.h"
#include <debug/debug.h>
#include <errno.h>
#include <fb/fb.h>
#include <klibc/mem.h>
#include <klibc/module.h>
#include <klibc/resource.h>

static char kbd_buffer[KBD_BUFFER_SIZE] = {0};
static size_t kbd_buffer_i = 0;
static char kbd_bigbuf[KBD_BIGBUF_SIZE] = {0};
static size_t kbd_bigbuf_i = 0;

static struct event console_event = {0};
static lock_t read_lock = {0};

static bool is_printable(uint8_t c) {
	return c >= 0x20 && c <= 0x7e;
}

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

static void add_to_buf_char(char c, bool echo) {
	if (c == '\n' && (console_device->term.c_iflag & ICRNL) == 0) {
		c = '\r';
	}

	if (console_device->term.c_lflag & ICANON) {
		switch (c) {
			case '\n': {
				if (kbd_buffer_i == KBD_BUFFER_SIZE) {
					return;
				}
				kbd_buffer[kbd_buffer_i++] = c;
				if (echo && (console_device->term.c_lflag & ECHO)) {
					framebuffer_puts("\n");
				}
				for (size_t i = 0; i < kbd_buffer_i; i++) {
					if ((console_device->res.status & POLLIN) == 0) {
						console_device->res.status |= POLLIN;
						event_trigger(&console_device->res.event, false);
					}
					if (kbd_bigbuf_i == KBD_BIGBUF_SIZE) {
						return;
					}
					kbd_bigbuf[kbd_bigbuf_i++] = kbd_buffer[i];
				}
				kbd_buffer_i = 0;
				return;
			}
			case '\b': {
				if (kbd_buffer_i == 0) {
					return;
				}
				kbd_buffer_i--;
				size_t to_backspace;
				if (kbd_buffer[kbd_buffer_i] >= 0x01 &&
					kbd_buffer[kbd_buffer_i] <= 0x1a) {
					to_backspace = 2;
				} else {
					to_backspace = 1;
				}
				kbd_buffer[kbd_buffer_i] = 0;
				if (echo && (console_device->term.c_lflag & ECHO) != 0) {
					for (size_t i = 0; i < to_backspace; i++) {
						framebuffer_putchar('\b');
						framebuffer_putchar(' ');
						framebuffer_putchar('\b');
					}
				}
				return;
			}
		}

		if (kbd_buffer_i == KBD_BUFFER_SIZE) {
			return;
		}
		kbd_buffer[kbd_buffer_i++] = c;
	} else {
		if ((console_device->res.status & POLLIN) == 0) {
			console_device->res.status |= POLLIN;
			event_trigger(&console_device->res.event, false);
		}
		if (kbd_bigbuf_i == KBD_BIGBUF_SIZE) {
			return;
		}
		kbd_bigbuf[kbd_bigbuf_i++] = c;
	}

	if (echo && (console_device->term.c_lflag & ECHO) != 0) {
		if (is_printable(c)) {
			framebuffer_putchar(c);
		} else if (c >= 0x01 && c <= 0x1a) {
			char caret[3] = {0};
			caret[0] = '^';
			caret[1] = c + 0x40;
			framebuffer_puts(caret);
		}
	}
}

void add_to_buf(char *ptr, size_t count, bool echo) {
	spinlock_acquire_or_wait(&read_lock);
	for (size_t i = 0; i < count; i++) {
		char c = ptr[i];
#if 0
        if ((console_res->termios.c_lflag & ISIG) != 0) {
            if (c == (char)console_res->termios.c_cc[VINTR]) {
                // Send signal
            }
        }
#endif
		add_to_buf_char(c, echo);
	}

	event_trigger(&console_event, false);
	spinlock_drop(&read_lock);
}

struct console *console_device = NULL;

static ssize_t console_read(struct resource *this,
							struct f_description *description, void *_buf,
							off_t offset, size_t count) {
	char *buf = _buf;

	while (!spinlock_acquire(&read_lock)) {
		struct event *events[] = {&console_event};
		if (event_await(events, 1, true) == -1) {
			errno = EINTR;
			return -1;
		}
	}

	bool wait = true;

	for (size_t i = 0; i < count;) {
		if (kbd_bigbuf_i != 0) {
			buf[i] = kbd_bigbuf[0];
			i++;
			kbd_bigbuf_i--;
			for (size_t j = 0; j < kbd_bigbuf_i; j++) {
				kbd_bigbuf[j] = kbd_bigbuf[j + 1];
			}
			if (kbd_bigbuf_i == 0 &&
				(console_device->res.status & POLLIN) != 0) {
				console_device->res.status &= ~POLLIN;
				event_trigger(&console_device->res.event, false);
			}
			wait = false;
		} else {
			if (wait == true) {
				spinlock_drop(&read_lock);
				for (;;) {
					struct event *events[] = {&console_event};
					if (event_await(events, 1, true) == -1) {
						errno = EINTR;
						return -1;
					}
					if (spinlock_acquire(&read_lock) == true) {
						break;
					}
				}
			} else {
				spinlock_drop(&read_lock);
				return i;
			}
		}
	}

	spinlock_drop(&read_lock);
	return count;
}

int console_ioctl(struct resource *this, struct f_description *description,
				  uint64_t request, uint64_t arg) {
	if (!arg) {
		return -1;
	}

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

uint64_t driver_entry(struct module *driver_module) {
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

	console_device->res.read = console_read;
	console_device->res.write = console_write;
	console_device->res.ioctl = console_ioctl;

	console_device->decckm = false;

	devtmpfs_add_device((struct resource *)console_device, "console");

	kprintffos(false, "Bye bye!\n");
	framebuffer_clear(0x00eee8d5, 0);
	framebuff.ctx->callback = term_callback;

	keyboard_init();

	return 0;
}
