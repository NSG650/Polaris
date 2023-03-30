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

#include <io/ports.h>
#include <locks/spinlock.h>
#include <serial/serial.h>
#include <stdbool.h>

lock_t serial_lock = 0;

void serial_init(void) {
	outb(COM1 + 1, 0x1);
	outb(COM1 + 3, 0x80);
	outb(COM1, 0x1);
	outb(COM1 + 1, 0x0);
	outb(COM1 + 3, 0x3);
	outb(COM1 + 2, 0xC7);
	outb(COM1 + 4, 0xB);
}

static inline bool is_transmit_empty(void) {
	return (inb(COM1 + 5) & 0b1000000) != 0;
}

static inline void transmit_data(uint8_t value) {
	while (!is_transmit_empty()) {
		asm volatile("pause");
	}
	outb(COM1, value);
}

void serial_putchar(char ch) {
	spinlock_acquire_or_wait(serial_lock);

	transmit_data(ch);

	spinlock_drop(serial_lock);
}

void serial_puts(char *str) {
	spinlock_acquire_or_wait(serial_lock);

	while (*str) {
		if (*str == '\n')
			transmit_data('\r');
		transmit_data(*str++);
	}

	spinlock_drop(serial_lock);
}
