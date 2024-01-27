#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <locks/spinlock.h>
#include <stddef.h>
#include <stdint.h>

#define KEY_PRESS_RELEASED (1)
#define KEY_LCTRL_DOWN (1 << 1)
#define KEY_RCTL_DOWN (1 << 2)
#define KEY_LALT_DOWN (1 << 3)
#define KEY_RALT_DOWN (1 << 4)
#define KEY_LSHIFT_DOWN (1 << 5)
#define KEY_RSHIFT_DOWN (1 << 6)
#define KEY_CAPS_ENABLED (1 << 7)

// linux /dev/input/event keycodes

#define KEYCODE_RESERVED 0
#define KEYCODE_ESCAPE 1
#define KEYCODE_1 2
#define KEYCODE_2 3
#define KEYCODE_3 4
#define KEYCODE_4 5
#define KEYCODE_5 6
#define KEYCODE_6 7
#define KEYCODE_7 8
#define KEYCODE_8 9
#define KEYCODE_9 10
#define KEYCODE_0 11
#define KEYCODE_MINUS 12
#define KEYCODE_EQUAL 13
#define KEYCODE_BACKSPACE 14
#define KEYCODE_TAB 15
#define KEYCODE_Q 16
#define KEYCODE_W 17
#define KEYCODE_E 18
#define KEYCODE_R 19
#define KEYCODE_T 20
#define KEYCODE_Y 21
#define KEYCODE_U 22
#define KEYCODE_I 23
#define KEYCODE_O 24
#define KEYCODE_P 25
#define KEYCODE_LEFTBRACE 26
#define KEYCODE_RIGHTBRACE 27
#define KEYCODE_ENTER 28
#define KEYCODE_LEFTCTRL 29
#define KEYCODE_A 30
#define KEYCODE_S 31
#define KEYCODE_D 32
#define KEYCODE_F 33
#define KEYCODE_G 34
#define KEYCODE_H 35
#define KEYCODE_J 36
#define KEYCODE_K 37
#define KEYCODE_L 38
#define KEYCODE_SEMICOLON 39
#define KEYCODE_APOSTROPHE 40
#define KEYCODE_GRAVE 41
#define KEYCODE_LEFTSHIFT 42
#define KEYCODE_BACKSLASH 43
#define KEYCODE_Z 44
#define KEYCODE_X 45
#define KEYCODE_C 46
#define KEYCODE_V 47
#define KEYCODE_B 48
#define KEYCODE_N 49
#define KEYCODE_M 50
#define KEYCODE_COMMA 51
#define KEYCODE_DOT 52
#define KEYCODE_SLASH 53
#define KEYCODE_RIGHTSHIFT 54
#define KEYCODE_KEYPADASTERISK 55
#define KEYCODE_LEFTALT 56
#define KEYCODE_SPACE 57
#define KEYCODE_CAPSLOCK 58
#define KEYCODE_F1 59
#define KEYCODE_F2 60
#define KEYCODE_F3 61
#define KEYCODE_F4 62
#define KEYCODE_F5 63
#define KEYCODE_F6 64
#define KEYCODE_F7 65
#define KEYCODE_F8 66
#define KEYCODE_F9 67
#define KEYCODE_F10 68
#define KEYCODE_NUMLOCK 69
#define KEYCODE_SCROLLLOCK 70
#define KEYCODE_KEYPAD7 71
#define KEYCODE_KEYPAD8 72
#define KEYCODE_KEYPAD9 73
#define KEYCODE_KEYPADMINUS 74
#define KEYCODE_KEYPAD4 75
#define KEYCODE_KEYPAD5 76
#define KEYCODE_KEYPAD6 77
#define KEYCODE_KEYPADPLUS 78
#define KEYCODE_KEYPAD1 79
#define KEYCODE_KEYPAD2 80
#define KEYCODE_KEYPAD3 81
#define KEYCODE_KEYPAD0 82
#define KEYCODE_KEYPADDOT 83

#define KEYCODE_F11 87
#define KEYCODE_F12 88

#define KEYCODE_KEYPADENTER 96
#define KEYCODE_RIGHTCTRL 97
#define KEYCODE_KEYPADSLASH 98
#define KEYCODE_SYSREQ 99
#define KEYCODE_RIGHTALT 100
#define KEYCODE_LINEFEED 101
#define KEYCODE_HOME 102
#define KEYCODE_UP 103
#define KEYCODE_PAGEUP 104
#define KEYCODE_LEFT 105
#define KEYCODE_RIGHT 106
#define KEYCODE_END 107
#define KEYCODE_DOWN 108
#define KEYCODE_PAGEDOWN 109
#define KEYCODE_INSERT 110
#define KEYCODE_DELETE 111

#define KEYBOARD_BUFFER_SIZE 1024

struct key_press {
	char ascii;
	int keycode;
	int flags;
};

struct key_press_buffer {
	struct key_press presses[KEYBOARD_BUFFER_SIZE];
	uintptr_t write_index;
	uintptr_t read_index;
	lock_t lock;
};

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
	[0x11] = KEYCODE_RIGHTALT,	  // altgr
	[0x14] = KEYCODE_RIGHTCTRL,
	[0x4A] = KEYCODE_KEYPADSLASH, // k/
	[0x5A] = KEYCODE_KEYPADENTER,
	[0x69] = KEYCODE_END,		  // end
	[0x6B] = KEYCODE_LEFT,		  // left
	[0x6C] = KEYCODE_HOME,		  // home
	[0x70] = KEYCODE_INSERT,	  // insert
	[0x71] = KEYCODE_DELETE,	  // delete
	[0x72] = KEYCODE_DOWN,		  // down
	[0x74] = KEYCODE_RIGHT,		  // right
	[0x75] = KEYCODE_UP,		  // up
	[0x7A] = KEYCODE_PAGEDOWN,	  // page down
	[0x7D] = KEYCODE_PAGEUP	  // page up
};

static char ascii_table[] = {
	0,	  '\033', '1', '2',	 '3', '4', '5', '6', '7', '8', '9', '0', '-',  '=',
	'\b', '\t',	  'q', 'w',	 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[',  ']',
	'\n', 0,	  'a', 's',	 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
	0,	  '\\',	  'z', 'x',	 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,	   '*',
	0,	  ' ',	  0,   0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	   0,
	0,	  '7',	  '8', '9',	 '-', '4', '5', '6', '+', '1', '2', '3', '0',  '.',
	0,	  0,	  0,   0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 '\r', 0,
	'/',  0,	  0,   '\r', 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	   0};

static char ascii_table_upper[] = {
	0,	  '\033', '!', '@',	 '#', '$', '%', '^', '&', '*', '(', ')', '_',  '+',
	'\b', '\t',	  'Q', 'W',	 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{',  '}',
	'\n', 0,	  'A', 'S',	 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~',
	0,	  '|',	  'Z', 'X',	 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,	   '*',
	0,	  ' ',	  0,   0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	   0,
	0,	  '7',	  '8', '9',	 '-', '4', '5', '6', '+', '1', '2', '3', '0',  '.',
	0,	  0,	  0,   0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 '\r', 0,
	'/',  0,	  0,   '\r', 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	   0};

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

#define SET_OR_UNSET_KEY(flags, keyboard_flag, released) \
	{                                                    \
		if (released) {                                  \
			flags &= ~keyboard_flag;                     \
		} else {                                         \
			flags |= keyboard_flag;                      \
		}                                                \
	}

#endif
