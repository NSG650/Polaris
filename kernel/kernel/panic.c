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
#include "../klibc/string.h"
#include "../serial/serial.h"
#include "../video/video.h"

void panic(char message[], char file[], char assert, uint32_t line) {
	char x[1024];
	uint8_t *rip = __builtin_return_address(0);
	uint64_t *rbp = __builtin_frame_address(0);
	if (assert) {
		clear_screen(0x00B800);
		sprintf(x,
				"*** ASSERTION FAILURE: %s\nFile: %s\nLine: %u\nRIP: "
				"0x%p\nRBP: 0x%p\n",
				message, file, line, rip, rbp);
		kprintbgc(x, 0xFFFFFF, 0x00B800);
	} else {
		clear_screen(0xB80000);
		sprintf(x, "*** PANIC: %s\nFile: %s\nLine: %u\nRIP: 0x%p\nRBP: 0x%p\n",
				message, file, line, rip, rbp);
		kprintbgc(x, 0xFFFFFF, 0xB80000);
	}
	write_serial(x);
	for (;;)
		__asm__("cli\nhlt");
}
