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

#ifndef DEBUG_H
#define DEBUG_H

#if defined(__x86_64__)
#include "../../arch/x86_64/include/serial/serial.h"
#define kputs_ serial_puts
#define kputchar_ serial_putchar
#endif

#include <sched/syscall.h>
#include <stdbool.h>

void kputchar(char c);
void kputs(char *string);
void kprintffos(bool fos, char *fmt, ...);
void panic_(size_t *ip, size_t *bp, char *fmt, ...);
void syscall_puts(struct syscall_arguments *args);
void backtrace(size_t *ip, size_t *bp);

#define panic(...) \
	panic_(__builtin_return_address(0), __builtin_frame_address(0), __VA_ARGS__)

#define kprintf(...) kprintffos(1, __VA_ARGS__)

#endif
