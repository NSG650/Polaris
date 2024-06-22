#include "console.h"
#include <asm/asm.h>
#include <debug/debug.h>
#include <io/ports.h>
#include <klibc/resource.h>
#include <sys/apic.h>
#include <sys/isr.h>

static const char convtab_capslock[] = {
	'\0', '\e', '1',  '2',	'3',  '4',	'5',  '6',	'7',  '8', '9', '0',
	'-',  '=',	'\b', '\t', 'Q',  'W',	'E',  'R',	'T',  'Y', 'U', 'I',
	'O',  'P',	'[',  ']',	'\n', '\0', 'A',  'S',	'D',  'F', 'G', 'H',
	'J',  'K',	'L',  ';',	'\'', '`',	'\0', '\\', 'Z',  'X', 'C', 'V',
	'B',  'N',	'M',  ',',	'.',  '/',	'\0', '\0', '\0', ' '};

static const char convtab_shift[] = {
	'\0', '\e', '!',  '@',	'#',  '$',	'%',  '^',	'&',  '*', '(', ')',
	'_',  '+',	'\b', '\t', 'Q',  'W',	'E',  'R',	'T',  'Y', 'U', 'I',
	'O',  'P',	'{',  '}',	'\n', '\0', 'A',  'S',	'D',  'F', 'G', 'H',
	'J',  'K',	'L',  ':',	'"',  '~',	'\0', '|',	'Z',  'X', 'C', 'V',
	'B',  'N',	'M',  '<',	'>',  '?',	'\0', '\0', '\0', ' '};

static const char convtab_shift_capslock[] = {
	'\0', '\e', '!',  '@',	'#',  '$',	'%',  '^',	'&',  '*', '(', ')',
	'_',  '+',	'\b', '\t', 'q',  'w',	'e',  'r',	't',  'y', 'u', 'i',
	'o',  'p',	'{',  '}',	'\n', '\0', 'a',  's',	'd',  'f', 'g', 'h',
	'j',  'k',	'l',  ':',	'"',  '~',	'\0', '|',	'z',  'x', 'c', 'v',
	'b',  'n',	'm',  '<',	'>',  '?',	'\0', '\0', '\0', ' '};

static const char convtab_nomod[] = {
	'\0', '\e', '1',  '2',	'3',  '4',	'5',  '6',	'7',  '8', '9', '0',
	'-',  '=',	'\b', '\t', 'q',  'w',	'e',  'r',	't',  'y', 'u', 'i',
	'o',  'p',	'[',  ']',	'\n', '\0', 'a',  's',	'd',  'f', 'g', 'h',
	'j',  'k',	'l',  ';',	'\'', '`',	'\0', '\\', 'z',  'x', 'c', 'v',
	'b',  'n',	'm',  ',',	'.',  '/',	'\0', '\0', '\0', ' '};

#define SCANCODE_MAX 0x57
#define SCANCODE_CTRL 0x1d
#define SCANCODE_CTRL_REL 0x9d
#define SCANCODE_SHIFT_RIGHT 0x36
#define SCANCODE_SHIFT_RIGHT_REL 0xb6
#define SCANCODE_SHIFT_LEFT 0x2a
#define SCANCODE_SHIFT_LEFT_REL 0xaa
#define SCANCODE_ALT_LEFT 0x38
#define SCANCODE_ALT_LEFT_REL 0xb8
#define SCANCODE_CAPSLOCK 0x3a
#define SCANCODE_NUMLOCK 0x45

static uint8_t keymap_state = 0;

#define CTRL_ACTIVE (1 << 0)
#define SHIFT_ACTIVE (1 << 1)
#define CAPSLOCK_ACTIVE (1 << 2)

static bool extra_scancodes = false;

static uint8_t ps2_read(void) {
	while ((inb(0x64) & 1) == 0)
		;
	return inb(0x60);
}

static int toupper(int c) {
	if (c >= 'a' && c <= 'z') {
		c -= 0x20;
	}
	return c;
}

static void keyboard_interrupt_handle(registers_t *r) {
	cli();

	uint8_t d = ps2_read();
	if (d == 0xe0) {
		extra_scancodes = true;
		goto end;
	}

	if (d == SCANCODE_CTRL) {
		keymap_state |= CTRL_ACTIVE;
		goto end;
	}

	if (d == SCANCODE_CTRL_REL) {
		keymap_state &= ~(CTRL_ACTIVE);
		goto end;
	}

	if (extra_scancodes) {
		extra_scancodes = false;
		switch (d) {
			case 0x1c:
				add_to_buf("\n", 1, true);
				goto end;
			case 0x35:
				add_to_buf("/", 1, true);
				goto end;
			case 0x48: // up
				if (!console_device->decckm) {
					add_to_buf("\e[A", 3, true);
				} else {
					add_to_buf("\eOA", 3, true);
				}
				goto end;
			case 0x4b: // left
				if (!console_device->decckm) {
					add_to_buf("\e[D", 3, true);
				} else {
					add_to_buf("\eOD", 3, true);
				}
				goto end;
			case 0x50: // down
				if (!console_device->decckm) {
					add_to_buf("\e[B", 3, true);
				} else {
					add_to_buf("\eOB", 3, true);
				}
				goto end;
			case 0x4d: // right
				if (!console_device->decckm) {
					add_to_buf("\e[C", 3, true);
				} else {
					add_to_buf("\eOC", 3, true);
				}
				goto end;
			case 0x47: // home
				add_to_buf("\e[1~", 4, true);
				goto end;
			case 0x4f: // end
				add_to_buf("\e[4~", 4, true);
				goto end;
			case 0x49: // pgup
				add_to_buf("\e[5~", 4, true);
				goto end;
			case 0x51: // pgdown
				add_to_buf("\e[6~", 4, true);
				goto end;
			case 0x53: // delete
				add_to_buf("\e[3~", 4, true);
				goto end;
		}
	}

	else {
		if (d == SCANCODE_SHIFT_LEFT || d == SCANCODE_SHIFT_RIGHT) {
			keymap_state |= SHIFT_ACTIVE;
			goto end;
		}

		if (d == SCANCODE_SHIFT_LEFT_REL || d == SCANCODE_SHIFT_RIGHT_REL) {
			keymap_state &= ~(SHIFT_ACTIVE);
			goto end;
		}

		if (d == SCANCODE_CAPSLOCK) {
			keymap_state ^= CAPSLOCK_ACTIVE;
			goto end;
		}
	}

	char c = 0;
	if (d < SCANCODE_MAX) {
		switch ((keymap_state >> 1) << 1) {
			case 0:
				c = convtab_nomod[d];
				break;
			case SHIFT_ACTIVE:
				c = convtab_shift[d];
				break;
			case CAPSLOCK_ACTIVE:
				c = convtab_capslock[d];
				break;
			case SHIFT_ACTIVE | CAPSLOCK_ACTIVE:
				c = convtab_shift_capslock[d];
				break;
			default:
				c = convtab_nomod[d];
				break;
		}
	} else {
		goto end;
	}

	if (keymap_state & CTRL_ACTIVE) {
		c = toupper(c) - 0x40;
	}

	add_to_buf(&c, 1, true);

end:
	sti();
	apic_eoi();
}

void keyboard_init(void) {
	isr_register_handler(49, keyboard_interrupt_handle);
	ioapic_redirect_irq(1, 49);
}