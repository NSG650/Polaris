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

#include <sys/idt.h>

struct idt_entry {
	uint16_t offset_lo;
	uint16_t selector;
	uint8_t ist;
	uint8_t flags;
	uint16_t offset_mid;
	uint32_t offset_hi;
	uint32_t zero;
} __attribute__((packed));

struct idt_register {
	uint16_t size;
	uint64_t address;
} __attribute__((packed));

struct idt_entry idt[256] = {0};

void idt_set_gate(size_t vec, void *handler, uint8_t ist) {
	uint64_t p = (uint64_t)handler;

	idt[vec].offset_lo = (uint16_t)p;
	idt[vec].selector = 8;
	idt[vec].ist = ist;
	idt[vec].flags = 0x8E;
	idt[vec].offset_mid = (uint16_t)(p >> 16);
	idt[vec].offset_hi = (uint32_t)(p >> 32);
	idt[vec].zero = 0;
}

void idt_reload(void) {
	struct idt_register idt_ptr = {sizeof(idt) - 1, (uint64_t)idt};

	asm volatile("lidtq %0" : : "m"(idt_ptr));
}
