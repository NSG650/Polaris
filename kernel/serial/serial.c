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

#include "serial.h"
#include "../cpu/ports.h"
#include "../klibc/lock.h"

lock_t serial_lock;

void serial_install(void) {
	port_byte_out(COM1, 0);
	port_byte_out(COM1 + 3, 0x80);
	port_byte_out(COM1, 3);
	port_byte_out(COM1 + 1, 0);
	port_byte_out(COM1 + 3, 3);
	port_byte_out(COM1 + 2, 0xC7);
	port_byte_out(COM1 + 4, 0xB);
	port_byte_out(COM1 + 4, 0xF);
}

static bool is_transmit_empty(void) {
	return port_byte_in(COM1 + 5) & 0x20;
}

void write_serial_char(char word) {
	while (!is_transmit_empty())
		;

	port_byte_out(COM1, word);
}

void write_serial(char *word) {
	LOCK(serial_lock);
	asm volatile("cli");

	while (*word != '\0') {
		write_serial_char(*word++);
	}

	UNLOCK(serial_lock);
	asm volatile("sti");
}
