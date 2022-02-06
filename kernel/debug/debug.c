/*
 * Copyright 2021, 2022 NSG650
 * Copyright 2021, 2022 Sebastian
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <asm/asm.h>
#include <sys/halt.h>
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
				char string[21] = {0};
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


void panic(char *fmt, ...) {
	kprintf("*** PANIC: ");
	va_list args;
	va_start(args, fmt);
	kprintf_(fmt, args);
	va_end(args);
	halt_other_cpus();
	halt_cpu0();
	halt_current_cpu();
	__builtin_unreachable();
}
