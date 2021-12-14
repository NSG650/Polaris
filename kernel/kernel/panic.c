/*
 * Copyright 2021 NSG650
 * Copyright 2021 Sebastian
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

#include "panic.h"
#include "../klibc/printf.h"
#include "../serial/serial.h"
#include "../video/video.h"
#include "symbols.h"

__attribute__((noreturn)) void panic(const char *message, char *file,
									 bool assert, size_t line) {
	asm("cli");
	size_t *rip = __builtin_return_address(0);
	size_t *rbp = __builtin_frame_address(0);
	if (assert) {
		clear_screen(0x00B800);
		printf("*** ASSERTION FAILURE: %s\nFile: %s\nLine: %zu\nRIP: "
			   "0x%p\nRBP: 0x%p\nKernel build: %s\n",
			   message, file, line, rip, rbp, KVERSION);
	} else {
		clear_screen(0xB80000);
		printf("*** PANIC: %s\nFile: %s\nLine: %zu\nRIP: 0x%p\nRBP: "
			   "0x%p\nKernel build: %s\n",
			   message, file, line, rip, rbp, KVERSION);
	}
	printf("Stack trace:\n");
	for (;;) {
		size_t old_rbp = rbp[0];
		size_t ret_address = rbp[1];
		if (!ret_address)
			break;
		printf("0x%llX\t%s\n", ret_address,
			   symbols_return_function_name(ret_address));
		if (!old_rbp)
			break;
		rbp = (void *)old_rbp;
	}
	for (;;)
		asm("cli\nhlt");
}
