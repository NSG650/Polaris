#include "keyboard.h"
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
	if (((buffer->write_index + 1) % KEYBOARD_BUFFER_SIZE) !=
		buffer->read_index) {
		buffer->presses[buffer->write_index] = *val;
		buffer->write_index = (buffer->write_index + 1) % KEYBOARD_BUFFER_SIZE;
		return true;
	}

	kprintf("Warning keyboard buffer is full\n");
	return false;
}

static void ringbuffer_read(struct key_press_buffer *buffer,
							struct key_press **val) {
	if (buffer->write_index == buffer->read_index) {
		*val = NULL;
		return;
	}
	*val = &buffer->presses[buffer->read_index];
	buffer->read_index = (buffer->read_index + 1) % KEYBOARD_BUFFER_SIZE;
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

	spinlock_acquire_or_wait(&keyboard_resource->lock);

	if (ringbuffer_write(&keyboard_buffer, press)) {
		event_trigger(&keyboard_resource->event, false);
	}

	spinlock_drop(&keyboard_resource->lock);
}

void keyboard_handle(registers_t *reg) {
	(void)reg;

	struct key_press press = {0};
	uint8_t scancode = inb(0x60);

	if (scancode == 0xE0) {
		extended = true;
		goto end;
	}

	if (scancode & 0x80) {
		press.flags = KEY_PRESS_RELEASED;
		scancode &= 0x7F;
	}

	char *table = codes;
	if (extended) {
		table = extendedcodes;
		extended = false;
	}

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

	uint8_t keyboard_config = keyboard_read_config();
	// Enable keyboard interrupt and keyboard scancode translation
	keyboard_config |= (1 << 0) | (1 << 6);
	// Enable mouse interrupt if any
	if ((keyboard_config & (1 << 5)) != 0) {
		keyboard_config |= (1 << 1);
	}
	keyboard_write_config(keyboard_config);

	// Enable keyboard port
	keyboard_write(0x64, 0xae);

	isr_register_handler(49, keyboard_handle);
	ioapic_redirect_irq(1, 49);
	inb(0x60);

	return 0;
}
