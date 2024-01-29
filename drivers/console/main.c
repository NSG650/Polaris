#include "keyboard.h"
#include "termios.h"
#include <asm/asm.h>
#include <debug/debug.h>
#include <fb/fb.h>
#include <fs/devtmpfs.h>
#include <klibc/mem.h>
#include <klibc/module.h>
#include <x86_64/io/ports.h>
#include <x86_64/sys/apic.h>
#include <x86_64/sys/isr.h>

#define POLLIN 0x0001
#define POLLOUT 0x0004

lock_t term_lock = {0};
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

struct console *console_device;

static int keyboard_flags = 0;

static bool ringbuffer_write(struct key_press_buffer *buffer,
							 struct key_press *val) {
	cli();
	bool ret = false;
	spinlock_acquire_or_wait(&buffer->lock);
	if (((buffer->write_index + 1) % KEYBOARD_BUFFER_SIZE) !=
		buffer->read_index) {
		buffer->presses[buffer->write_index] = *val;
		buffer->write_index = (buffer->write_index + 1) % KEYBOARD_BUFFER_SIZE;
		ret = true;
		goto end;
	}

	kprintf("Warning: Keyboard buffer is full\n");
end:
	spinlock_drop(&buffer->lock);
	sti();
	return ret;
}

static void ringbuffer_read(struct key_press_buffer *buffer,
							struct key_press **val) {
	cli();
	spinlock_acquire_or_wait(&buffer->lock);
	if (buffer->write_index == buffer->read_index) {
		*val = NULL;
		goto end;
	}
	*val = &buffer->presses[buffer->read_index];
	buffer->read_index = (buffer->read_index + 1) % KEYBOARD_BUFFER_SIZE;
end:
	spinlock_drop(&buffer->lock);
	sti();
}

static bool extended = false;
struct key_press_buffer keyboard_buffer = {0};

static uint8_t keyboard_read(void) {
	while ((inb(0x64) & 1) == 0)
		;

	return inb(0x60);
}

static void keyboard_write(uint16_t port, uint8_t value) {
	while ((inb(0x64) & 2) != 0)
		;

	outb(port, value);
}

static uint8_t keyboard_read_config(void) {
	keyboard_write(0x64, 0x20);
	return keyboard_read();
}

static void keyboard_write_config(uint8_t value) {
	keyboard_write(0x64, 0x60);
	keyboard_write(0x60, value);
}

static ssize_t console_write(struct resource *_this,
							 struct f_description *description, const void *buf,
							 off_t offset, size_t count);

static void keyboard_add_to_buffer_char(struct key_press *press, bool echo) {
	bool released = press->flags & KEY_PRESS_RELEASED;

	switch (press->keycode) {
		case KEYCODE_LEFTCTRL:
			SET_OR_UNSET_KEY(keyboard_flags, KEY_LCTRL_DOWN, released);
			break;
		case KEYCODE_RIGHTCTRL:
			SET_OR_UNSET_KEY(keyboard_flags, KEY_RCTL_DOWN, released);
			break;
		case KEYCODE_LEFTSHIFT:
			SET_OR_UNSET_KEY(keyboard_flags, KEY_LSHIFT_DOWN, released);
			break;
		case KEYCODE_RIGHTSHIFT:
			SET_OR_UNSET_KEY(keyboard_flags, KEY_RSHIFT_DOWN, released);
			break;
		case KEYCODE_LEFTALT:
			SET_OR_UNSET_KEY(keyboard_flags, KEY_LALT_DOWN, released);
			break;
		case KEYCODE_RIGHTALT:
			SET_OR_UNSET_KEY(keyboard_flags, KEY_RALT_DOWN, released);
			break;
		case KEYCODE_CAPSLOCK:
			SET_OR_UNSET_KEY(keyboard_flags, KEY_CAPS_ENABLED, released);
			break;
	}

	if (released == true) {
		return;
	}

	press->flags |= keyboard_flags;

	char *table = ascii_table;
	if ((press->flags & KEY_LSHIFT_DOWN) || (press->flags & KEY_RSHIFT_DOWN)) {
		table = ascii_table_upper;
	}

	press->ascii = table[press->keycode];

	if ((press->flags & KEY_LCTRL_DOWN) || (press->flags & KEY_RCTL_DOWN)) {
		press->ascii = toupper(press->ascii) - 0x40;
	}

	if (press->ascii == '\n' &&
		(console_device->info.termios.c_iflag & ICRNL) == 0) {
		press->ascii = '\r';
	}

	if (console_device->info.termios.c_lflag & ICANON) {
		if (press->ascii == '\b') {
			if (keyboard_buffer.write_index == 0) {
				return;
			}

			keyboard_buffer.write_index--;
			if (keyboard_buffer.read_index == keyboard_buffer.write_index + 1) {
				keyboard_buffer.read_index = keyboard_buffer.write_index;
			}

			char key =
				keyboard_buffer.presses[keyboard_buffer.write_index].ascii;

			uint8_t to_backspace = 0;
			if (key >= 0x01 && key <= 0x1a) {
				to_backspace = 2;
			} else {
				to_backspace = 1;
			}

			if (echo && (console_device->info.termios.c_lflag & ECHO) != 0) {
				for (uint8_t i = 0; i < to_backspace; i++) {
					console_write(NULL, NULL, "\b \b", 0, 3);
				}
			}
			return;
		}

		ringbuffer_write(&keyboard_buffer, press);
	} else {
		if ((console_device->res.status & POLLIN) == 0) {
			console_device->res.status |= POLLIN;
			event_trigger(&console_device->res.event, false);
		}
		ringbuffer_write(&keyboard_buffer, press);
	}

	if (echo && (console_device->info.termios.c_lflag & ECHO) != 0) {
		if (is_printable(press->ascii)) {
			console_write(NULL, NULL, &press->ascii, 0, 1);
		} else if (press->ascii >= 0x01 && press->ascii <= 0x1a) {
			char caret[2];
			caret[0] = '^';
			caret[1] = press->ascii + 0x40;
			console_write(NULL, NULL, caret, 0, 2);
		}
	}
}

static void keyboard_add_to_buffer(struct key_press **presses, size_t count,
								   bool echo) {
	for (size_t i = 0; i < count; i++) {
		struct key_press *press = presses[i];
		keyboard_add_to_buffer_char(press, echo);
	}

	event_trigger(&console_device->res.event, false);
}

static bool release = false;

void keyboard_handle(registers_t *reg) {
	(void)reg;

	struct key_press press = {0};
	uint8_t scancode = inb(0x60);

	if (scancode == 0xE0) {
		extended = true;
		goto end;
	}

	if (scancode == 0xF0) {
		release = true;
		goto end;
	}

	if (release) {
		press.flags = KEY_PRESS_RELEASED;
		release = false;
	}

	char *table = codes;
	if (extended) {
		table = extendedcodes;
		extended = false;
	}

	scancode = translate_table[scancode];
	press.keycode = table[scancode];

	if (!press.keycode) {
		goto end;
	}

	struct key_press *arg = &press;
	keyboard_add_to_buffer(&arg, 1, true);
end:
	apic_eoi();
}

static ssize_t console_read(struct resource *this,
							struct f_description *description, void *_buf,
							off_t offset, size_t count) {
	char *buf = (char *)_buf;

	if (description->flags & O_NONBLOCK) {
		// errno = EWOULDBLOCK;
		return -1;
	}

	while (spinlock_acquire(&this->lock) == false) {
		struct event *events[] = {&console_device->res.event};
		if (event_await(events, 1, true) == -1) {
			// errno = EINTR;
			return -1;
		}
	}

	bool wait = true;

	size_t i = 0;
	while (i < count) {
		struct key_press *press = NULL;
		ringbuffer_read(&keyboard_buffer, &press);
		if (!press) {
			if (wait == true) {
				spinlock_drop(&this->lock);
				for (;;) {
					struct event *events[] = {&console_device->res.event};
					if (event_await(events, 1, true) == -1) {
						// errno = EINTR;
						return -1;
					}
					if (spinlock_acquire(&this->lock) == true) {
						break;
					}
				}
			} else {
				spinlock_drop(&this->lock);
				return i;
			}
		} else {
			buf[i] = press->ascii;
			i++;
		}
	}

	spinlock_drop(&this->lock);
	return i;
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

static void keyboard_init(void) {
	// Disable primary and secondary PS/2 ports
	keyboard_write(0x64, 0xad);
	keyboard_write(0x64, 0xa7);

	// Flush output buffer
	inb(0x60);

	uint8_t keyboard_config = keyboard_read_config();
	// Enable keyboard interrupt
	// and disable keyboard scancode translation
	keyboard_config |= (1 << 0);
	keyboard_config &= ~(1 << 6);
	// Enable mouse interrupt if it exists
	if ((keyboard_config & (1 << 5)) != 0) {
		keyboard_config |= (1 << 1);
	}

	// Test PS/2 controller, returns 0xFC if failed
	keyboard_write(0x64, 0xAA);
	if (keyboard_read() == 0xFC) {
		panic("Keyboard initialization failed\n");
	}

	keyboard_write_config(keyboard_config);

	// Test keyboard port, returns 0 if passed
	keyboard_write(0x64, 0xAB);
	if (keyboard_read() != 0) {
		panic("Keyboard initialization failed\n");
	}

	// Enable keyboard port
	keyboard_write(0x64, 0xae);

	isr_register_handler(49, keyboard_handle);
	ioapic_redirect_irq(1, 49);
	inb(0x60);
}

uint64_t driver_entry(struct module *driver_module) {
	strncpy(driver_module->name, "console", sizeof(driver_module->name));

	framebuff = framebuffer_info();
	if (!framebuff) {
		return 2;
	}

	framebuffer_set_callback(term_callback);

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

	console_device->res.status |= POLLOUT;

	console_device->res.read = console_read;
	console_device->res.write = console_write;
	console_device->res.ioctl = console_ioctl;

	devtmpfs_add_device((struct resource *)console_device, "console");

	keyboard_init();

	return 0;
}
