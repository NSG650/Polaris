/*
 * Copyright 2021 - 2023 NSG650
 * Copyright 2021 - 2023 Neptune
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
#include <debug/printf.h>
#include <fb/fb.h>
#include <klibc/kargs.h>
#include <klibc/mem.h>
#include <locks/spinlock.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/halt.h>
#include <sys/prcb.h>
#include <sys/timer.h>

static lock_t write_lock = {0};
bool in_panic = false;
bool put_to_fb = true;
bool print_now = false;

void kputchar(char c) {
	if (c == '\n')
		kputchar('\r');
	kputchar_(c);

	if (put_to_fb)
		framebuffer_putchar(c);
}

void kputs(char *string) {
	kputs_(string);

	if (put_to_fb)
		framebuffer_puts(string);
}

void syscall_puts(struct syscall_arguments *args) {
	if (kernel_arguments.kernel_args &
		KERNEL_ARGS_SUPPRESS_USER_DEBUG_MESSAGES) {
		return;
	}
	char *str = (char *)(args->args0);
	spinlock_acquire_or_wait(&write_lock);
	if (str) {
		kputs(str);
	}
	spinlock_drop(&write_lock);
}

bool disable_prefix = false;

void kprintffos(bool fos, char *fmt, ...) {
	spinlock_acquire_or_wait(&write_lock);
	if (!fos)
		put_to_fb = 0;
	va_list args;
	va_start(args, fmt);
	if (!print_now) {
		goto end;
	}
	if (!disable_prefix) {
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
	}
	vprintf_(fmt, args);
end:
	va_end(args);
	spinlock_drop(&write_lock);
}

void debug_hex_dump(const void *data, size_t size) {
	disable_prefix = true;
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
		kprintffos(0, "%02X ", ((unsigned char *)data)[i]);
		if (((unsigned char *)data)[i] >= ' ' &&
			((unsigned char *)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char *)data)[i];
		} else {
			ascii[i % 16] = '.';
		}
		if ((i + 1) % 8 == 0 || i + 1 == size) {
			kprintffos(0, " ");
			if ((i + 1) % 16 == 0) {
				kprintffos(0, "|  %s \n", ascii);
			} else if (i + 1 == size) {
				ascii[(i + 1) % 16] = '\0';
				if ((i + 1) % 16 <= 8) {
					kprintffos(0, " ");
				}
				for (j = (i + 1) % 16; j < 16; ++j) {
					kprintffos(0, "   ");
				}
				kprintffos(0, "|  %s \n", ascii);
			}
		}
	}
	disable_prefix = false;
}

void panic_(size_t *ip, size_t *bp, char *fmt, ...) {
	cli();
	spinlock_drop(&write_lock);
	halt_other_cpus();
	put_to_fb = 1;
	extern bool is_smp;
	extern bool sched_runit;
	if (in_panic) {
		kprintf("Pretty bad kernel panic here\n");
		va_list args;
		va_start(args, fmt);
		kputs("*** PANIC:\t");
		vprintf_(fmt, args);
		va_end(args);
		halt_current_cpu();
	}
	in_panic = true;

	if (is_smp) {
		kprintf("Panic called on CPU%d\n",
				prcb_return_current_cpu()->cpu_number);
	}

	if (sched_runit && prcb_return_current_cpu()->running_thread) {
		kprintf("Current thread id: %ld\n",
				prcb_return_current_cpu()->running_thread->tid);
		kprintf("Process name corresponding to the current thread: %s\n",
				prcb_return_current_cpu()->running_thread->mother_proc->name);
	}

	va_list args;
	va_start(args, fmt);
	kputs("*** PANIC:\t");
	vprintf_(fmt, args);
	va_end(args);
	kprintf("Crashed at %p\n", ip);
	backtrace(bp);
	halt_current_cpu();
	__builtin_unreachable();
}
