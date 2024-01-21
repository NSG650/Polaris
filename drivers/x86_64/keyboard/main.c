#include "keyboard.h"
#include <asm/asm.h>
#include <debug/debug.h>
#include <fb/fb.h>
#include <fs/devtmpfs.h>
#include <klibc/mem.h>
#include <klibc/module.h>
#include <sched/sched.h>
#include <sys/prcb.h>
#include <x86_64/io/ports.h>
#include <x86_64/sys/apic.h>
#include <x86_64/sys/isr.h>

static struct resource *keyboard_resource = NULL;
static int keyboard_flags = 0;

#define print_var_num(var) kprintffos(0, #var ": %d\n", var);

static bool ringbuffer_write(struct key_press_buffer *buffer,
							 struct key_press *val) {
	cli();
	spinlock_acquire_or_wait(&buffer->lock);
	if (((buffer->write_index + 1) % KEYBOARD_BUFFER_SIZE) !=
		buffer->read_index) {
		buffer->presses[buffer->write_index] = *val;
		buffer->write_index = (buffer->write_index + 1) % KEYBOARD_BUFFER_SIZE;
		spinlock_drop(&buffer->lock);
		sti();
		return true;
	}

	kprintf("Warning keyboard buffer is full\n");
	spinlock_drop(&buffer->lock);
	sti();
	return false;
}

static void ringbuffer_read(struct key_press_buffer *buffer,
							struct key_press **val) {
	cli();
	spinlock_acquire_or_wait(&buffer->lock);
	if (buffer->write_index == buffer->read_index) {
		*val = NULL;
		spinlock_drop(&buffer->lock);
		sti();
		return;
	}
	*val = &buffer->presses[buffer->read_index];
	buffer->read_index = (buffer->read_index + 1) % KEYBOARD_BUFFER_SIZE;
	spinlock_drop(&buffer->lock);
	sti();
}

static char codes[128] = {KEYCODE_RESERVED,
						  KEYCODE_ESCAPE,
						  KEYCODE_1,
						  KEYCODE_2,
						  KEYCODE_3,
						  KEYCODE_4,
						  KEYCODE_5,
						  KEYCODE_6,
						  KEYCODE_7,
						  KEYCODE_8,
						  KEYCODE_9,
						  KEYCODE_0,
						  KEYCODE_MINUS,
						  KEYCODE_EQUAL,
						  KEYCODE_BACKSPACE,
						  KEYCODE_TAB,
						  KEYCODE_Q,
						  KEYCODE_W,
						  KEYCODE_E,
						  KEYCODE_R,
						  KEYCODE_T,
						  KEYCODE_Y,
						  KEYCODE_U,
						  KEYCODE_I,
						  KEYCODE_O,
						  KEYCODE_P,
						  KEYCODE_LEFTBRACE,
						  KEYCODE_RIGHTBRACE,
						  KEYCODE_ENTER,
						  KEYCODE_LEFTCTRL,
						  KEYCODE_A,
						  KEYCODE_S,
						  KEYCODE_D,
						  KEYCODE_F,
						  KEYCODE_G,
						  KEYCODE_H,
						  KEYCODE_J,
						  KEYCODE_K,
						  KEYCODE_L,
						  KEYCODE_SEMICOLON,
						  KEYCODE_APOSTROPHE,
						  KEYCODE_GRAVE,
						  KEYCODE_LEFTSHIFT,
						  KEYCODE_BACKSLASH,
						  KEYCODE_Z,
						  KEYCODE_X,
						  KEYCODE_C,
						  KEYCODE_V,
						  KEYCODE_B,
						  KEYCODE_N,
						  KEYCODE_M,
						  KEYCODE_COMMA,
						  KEYCODE_DOT,
						  KEYCODE_SLASH,
						  KEYCODE_RIGHTSHIFT,
						  KEYCODE_KEYPADASTERISK,
						  KEYCODE_LEFTALT,
						  KEYCODE_SPACE,
						  KEYCODE_CAPSLOCK,
						  KEYCODE_F1,
						  KEYCODE_F2,
						  KEYCODE_F3,
						  KEYCODE_F4,
						  KEYCODE_F5,
						  KEYCODE_F6,
						  KEYCODE_F7,
						  KEYCODE_F8,
						  KEYCODE_F9,
						  KEYCODE_F10,
						  KEYCODE_NUMLOCK,
						  KEYCODE_SCROLLLOCK,
						  KEYCODE_KEYPAD7,
						  KEYCODE_KEYPAD8,
						  KEYCODE_KEYPAD9,
						  KEYCODE_KEYPADMINUS,
						  KEYCODE_KEYPAD4,
						  KEYCODE_KEYPAD5,
						  KEYCODE_KEYPAD6,
						  KEYCODE_KEYPADPLUS,
						  KEYCODE_KEYPAD1,
						  KEYCODE_KEYPAD2,
						  KEYCODE_KEYPAD3,
						  KEYCODE_KEYPAD0,
						  KEYCODE_KEYPADDOT,
						  0,
						  0,
						  0,
						  KEYCODE_F11,
						  KEYCODE_F12};

static char extendedcodes[128] = {
	[0x1C] = KEYCODE_KEYPADENTER, [0x1D] = KEYCODE_RIGHTCTRL,
	[0x35] = KEYCODE_KEYPADSLASH, // k/
	[0x38] = KEYCODE_RIGHTALT,	  // altgr
	[0x47] = KEYCODE_HOME,		  // home
	[0x48] = KEYCODE_UP,		  // up
	[0x49] = KEYCODE_PAGEUP,	  // page up
	[0x4B] = KEYCODE_LEFT,		  // left
	[0x4D] = KEYCODE_RIGHT,		  // right
	[0x4F] = KEYCODE_END,		  // end
	[0x50] = KEYCODE_DOWN,		  // down
	[0x51] = KEYCODE_PAGEDOWN,	  // page down
	[0x52] = KEYCODE_INSERT,	  // insert
	[0x53] = KEYCODE_DELETE		  // delete
};

static char ascii_table[] = {
	0,	  '\033', '1', '2',	 '3', '4', '5', '6', '7', '8', '9', '0', '-',  '=',
	'\b', '\t',	  'q', 'w',	 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[',  ']',
	'\r', 0,	  'a', 's',	 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
	0,	  '\\',	  'z', 'x',	 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,	   '*',
	0,	  ' ',	  0,   0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	   0,
	0,	  '7',	  '8', '9',	 '-', '4', '5', '6', '+', '1', '2', '3', '0',  '.',
	0,	  0,	  0,   0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 '\r', 0,
	'/',  0,	  0,   '\r', 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	   0};

static char ascii_table_upper[] = {
	0,	  '\033', '!', '@',	 '#', '$', '%', '^', '&', '*', '(', ')', '_',  '+',
	'\b', '\t',	  'Q', 'W',	 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{',  '}',
	'\r', 0,	  'A', 'S',	 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~',
	0,	  '|',	  'Z', 'X',	 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,	   '*',
	0,	  ' ',	  0,   0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	   0,
	0,	  '7',	  '8', '9',	 '-', '4', '5', '6', '+', '1', '2', '3', '0',  '.',
	0,	  0,	  0,   0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 '\r', 0,
	'/',  0,	  0,   '\r', 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	   0};

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

#define SET_OR_UNSET_KEY(flags, keyboard_flag, released) \
	{                                                    \
		if (released) {                                  \
			flags &= ~keyboard_flag;                     \
		} else {                                         \
			flags |= keyboard_flag;                      \
		}                                                \
	}

static void keyboard_add_to_buffer(struct key_press *press) {
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

	press->flags |= keyboard_flags;

	char *table = ascii_table;
	if ((press->flags & KEY_LSHIFT_DOWN) || (press->flags & KEY_RSHIFT_DOWN)) {
		table = ascii_table_upper;
	}

	press->ascii = table[press->keycode];

	if (ringbuffer_write(&keyboard_buffer, press)) {
		event_trigger(&keyboard_resource->event, false);
	}
}

static bool release = false;

static uint8_t translate_table[256] = {
	0xff, 0x43, 0x41, 0x3f, 0x3d, 0x3b, 0x3c, 0x58, 0x64, 0x44, 0x42, 0x40,
	0x3e, 0x0f, 0x29, 0x59, 0x65, 0x38, 0x2a, 0x70, 0x1d, 0x10, 0x02, 0x5a,
	0x66, 0x71, 0x2c, 0x1f, 0x1e, 0x11, 0x03, 0x5b, 0x67, 0x2e, 0x2d, 0x20,
	0x12, 0x05, 0x04, 0x5c, 0x68, 0x39, 0x2f, 0x21, 0x14, 0x13, 0x06, 0x5d,
	0x69, 0x31, 0x30, 0x23, 0x22, 0x15, 0x07, 0x5e, 0x6a, 0x72, 0x32, 0x24,
	0x16, 0x08, 0x09, 0x5f, 0x6b, 0x33, 0x25, 0x17, 0x18, 0x0b, 0x0a, 0x60,
	0x6c, 0x34, 0x35, 0x26, 0x27, 0x19, 0x0c, 0x61, 0x6d, 0x73, 0x28, 0x74,
	0x1a, 0x0d, 0x62, 0x6e, 0x3a, 0x36, 0x1c, 0x1b, 0x75, 0x2b, 0x63, 0x76,
	0x55, 0x56, 0x77, 0x78, 0x79, 0x7a, 0x0e, 0x7b, 0x7c, 0x4f, 0x7d, 0x4b,
	0x47, 0x7e, 0x7f, 0x6f, 0x52, 0x53, 0x50, 0x4c, 0x4d, 0x48, 0x01, 0x45,
	0x57, 0x4e, 0x51, 0x4a, 0x37, 0x49, 0x46, 0x54, 0x80, 0x81, 0x82, 0x41,
	0x54, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b,
	0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3,
	0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb,
	0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
	0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3,
	0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb,
	0xfc, 0xfd, 0xfe, 0xff,
};

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

	keyboard_add_to_buffer(&press);
end:
	apic_eoi();
}

void driver_exit(void) {
	isr_register_handler(49, NULL);
	kprintf("Bye!\n");
}

uint64_t driver_entry(struct module *driver_module) {
	strncpy(driver_module->name, "ps2keyboard", sizeof(driver_module->name));
	driver_module->exit = driver_exit;

	keyboard_resource = resource_create(sizeof(struct resource));
	keyboard_resource->stat.st_size = 0;
	keyboard_resource->stat.st_blocks = 0;
	keyboard_resource->stat.st_blksize = 4096;
	keyboard_resource->stat.st_rdev = resource_create_dev_id();
	keyboard_resource->stat.st_mode = 0644 | S_IFCHR;

	devtmpfs_add_device(keyboard_resource, "keyboard");

	spinlock_drop(&keyboard_resource->lock);

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

	return 0;
}
