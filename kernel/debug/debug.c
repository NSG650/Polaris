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

#include <asm/asm.h>
#include <debug/debug.h>
#include <fb/fb.h>
#include <klibc/mem.h>
#include <locks/spinlock.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/halt.h>
#include <sys/prcb.h>
#include <sys/timer.h>

lock_t write_lock;
bool in_panic = false;

void kputchar(char c) {
	spinlock_acquire_or_wait(write_lock);
	if (c == '\n')
		kputchar('\r');
	kputchar_(c);
	framebuffer_putchar(c);
	spinlock_drop(write_lock);
}

void kputs(char *string) {
	spinlock_acquire(write_lock);
	kputs_(string);
	framebuffer_puts(string);
	spinlock_drop(write_lock);
}

static void kprintf_(char *fmt, va_list args) {
	if (in_panic) {
		kputs("*** PANIC:\t");
	} else {
		uint64_t timer_tick = 0;
		if (timer_installed()) {
			timer_tick = timer_count();
		}
		char string[21] = {0};
		ltoa(timer_tick, string, 10);
		kputs("[");
		kputs(string);
		kputs("] ");
	}
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
				char string[20] = {0};
				uint64_t number = va_arg(args, size_t);
				ultoa(number, string, 16);
				kputs(string);
			}
			if (*fmt == 'd') {
				char string[21] = {0};
				uint64_t number = va_arg(args, size_t);
				ltoa(number, string, 10);
				kputs(string);
			}
			if (*fmt == 'u') {
				char string[21] = {0};
				uint64_t number = va_arg(args, size_t);
				ultoa(number, string, 10);
				kputs(string);
			}
		} else {
			kputchar(*fmt);
		}
		fmt++;
	}
}

void kprintf(char *fmt, ...) {
	spinlock_acquire_or_wait(write_lock);
	va_list args;
	va_start(args, fmt);
	kprintf_(fmt, args);
	va_end(args);
	spinlock_drop(write_lock);
}

void panic(char *fmt, ...) {
	cli();
	extern bool is_smp;
	if (in_panic) {
		halt_other_cpus();
		halt_current_cpu();
	}
	in_panic = true;
	if (is_smp)
		kprintf("Panic called on CPU%d\n",
				prcb_return_current_cpu()->cpu_number);
	halt_other_cpus();
	va_list args;
	va_start(args, fmt);
	kprintf_(fmt, args);
	va_end(args);
	size_t *ip = __builtin_return_address(0);
	size_t *bp = __builtin_frame_address(0);
	kprintf("Crashed at 0x%p\n", ip);
	kprintf("Stack trace:\n");
#if defined(__x86_64__)
	for (;;) {
		size_t old_rbp = bp[0];
		size_t ret_address = bp[1];
		if (!ret_address)
			break;
		kprintf("0x%p\n", ret_address);
		if (!old_rbp)
			break;
		bp = (void *)old_rbp;
	}
#endif
	halt_current_cpu();
	__builtin_unreachable();
}
