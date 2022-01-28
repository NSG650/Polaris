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


#include <io/ports.h>
#include <serial/serial.h>
#include <stdbool.h>

void serial_init(void) {
	outportb(COM1, 0);
	outportb(COM1 + 3, 0x80);
	outportb(COM1, 3);
	outportb(COM1 + 1, 0);
	outportb(COM1 + 3, 3);
	outportb(COM1 + 2, 0xC7);
	outportb(COM1 + 4, 0xB);
	outportb(COM1 + 4, 0xF);
}

static bool is_transmit_empty(void) {
	return inportb(COM1 + 5) & 0x20;
}

void serial_putchar(char c) {
	while (!is_transmit_empty())
		;

	outportb(COM1, c);
}

void serial_puts(char *string) {
	while (*string != '\0') {
		if (*string == '\n')
			serial_putchar('\r');
		serial_putchar(*string++);
	}
}
