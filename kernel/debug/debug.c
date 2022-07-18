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
#if defined(__x86_64__)
#include "../arch/x86_64/include/mm/vmm.h"
#endif

lock_t write_lock;
bool in_panic = false;
bool put_to_fb = true;

void kputchar(char c) {
	spinlock_acquire_or_wait(write_lock);
	if (c == '\n')
		kputchar('\r');
	kputchar_(c);
	if (put_to_fb)
		framebuffer_putchar(c);
	spinlock_drop(write_lock);
}

void kputs(char *string) {
	spinlock_acquire(write_lock);
	kputs_(string);
	if (put_to_fb)
		framebuffer_puts(string);
	spinlock_drop(write_lock);
}

void syscall_puts(struct syscall_arguments *args) {
	put_to_fb = true;
	if (args->args0)
		kputs((char *)args->args0);
}

static void kprintf_(char *fmt, va_list args) {
	if (in_panic) {
		kputs("*** PANIC:\t");
	} else if (put_to_fb) {
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
	for (;;) {
		while (*fmt && *fmt != '%')
			kputchar(*fmt++);

		if (!*fmt++)
			return;

		switch (*fmt++) {
			case '%':
				kputchar('%');
				break;
			case 's': {
				char *str = (char *)va_arg(args, const char *);
				if (!str)
					kputs("(null)");
				else
					kputs(str);
				break;
			}
			case 'S': {
				char *str = (char *)va_arg(args, const char *);
				size_t len = va_arg(args, size_t);
				if (!str)
					kputs("(null)");
				else
					for (size_t i = 0; i < len; i++)
						kputchar(str[i]);
				break;
			}
			case 'x':
			case 'p': {
				char string[20] = {0};
				uint64_t number = va_arg(args, size_t);
				ultoa(number, string, 16);
				kputs(string);
				break;
			}
			case 'd': {
				char string[21] = {0};
				uint64_t number = va_arg(args, size_t);
				ltoa(number, string, 10);
				kputs(string);
				break;
			}
			case 'u': {
				char string[21] = {0};
				uint64_t number = va_arg(args, size_t);
				ultoa(number, string, 10);
				kputs(string);
				break;
			}
			case 'c':
				kputchar((char)va_arg(args, int));
				break;
			default:
				kputchar('?');
				break;
		}
	}
}

void kprintffos(bool fos, char *fmt, ...) {
	if (!fos)
		put_to_fb = 0;
	spinlock_acquire_or_wait(write_lock);
	va_list args;
	va_start(args, fmt);
	kprintf_(fmt, args);
	va_end(args);
	spinlock_drop(write_lock);
	put_to_fb = 1;
}

void panic_(size_t *ip, size_t *bp, char *fmt, ...) {
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
