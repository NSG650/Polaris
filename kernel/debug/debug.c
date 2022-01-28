#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <debug/debug.h>
#include <fb/fb.h>

void kputchar(char c) {
	if (c == '\n')
		kputchar('\r');
	kputchar_(c);
	framebuffer_putchar(c);
}

void kputs(char *string) {
	kputs_(string);
	framebuffer_puts(string);
}

static void kprintf_(char *fmt, va_list args) {
	while (*fmt) {
		if (*fmt == '%') {
			fmt++;
			if (*fmt == '%') {
				kputchar('%');
			}
			if (*fmt == 's') {
				kputs(va_arg(args, char *));
			}
			if (*fmt == 'S') {
				const char *str = va_arg(args, char *);
				size_t len = va_arg(args, size_t);
				for (size_t i = 0; i < len; i++) {
					kputchar(str[i]);
				}
			}
			if (*fmt == 'x' || *fmt == 'p') {
				char string[20];
				uint64_t number = va_arg(args, size_t);
				for (int i = 16; i > 0; number >>= 4) {
					string[--i] = "0123456789ABCDEF"[number & 0x0f];
				}
				kputs(string);
			}
			if (*fmt == 'd') {
				char string[21];
				uint64_t number = va_arg(args, size_t);
				for (int i = 20; i > 0;) {
					string[--i] = number % 10 + '0';
					number /= 10;
				}
				size_t counter = 0;
				while (string[counter] == '0' && counter < 19) {
					counter++;
				}
				kputs(&string[counter]);
			}
		} else {
			kputchar(*fmt);
		}
		fmt++;
	}
}

void kprintf(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	kprintf_(fmt, args);
	va_end(args);
}
