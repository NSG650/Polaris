/*
 * Copyright 2021 NSG650
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

#include "gdt.h"
#include <stdint.h>

struct GDTEntry {
	uint16_t limit;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t access;
	uint8_t granularity;
	uint8_t base_hi;
} __attribute__((packed));

struct TSSEntry {
	uint16_t length;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t flags1;
	uint8_t flags2;
	uint8_t base_hi;
	uint32_t base_upper32;
	uint32_t reserved;
} __attribute__((packed));

struct GDT {
	struct GDTEntry entries[5];
	struct TSSEntry tss;
} __attribute__((packed));

struct GDTPointer {
	uint16_t limit;
	uint64_t ptr;
} __attribute__((packed));

struct GDT gdt;
struct GDTPointer gdt_pointer;

extern void gdt_reload(void);
extern void tss_reload(void);

void gdt_init(void) {
	// Null descriptor
	gdt.entries[0].limit = 0;
	gdt.entries[0].base_low = 0;
	gdt.entries[0].base_mid = 0;
	gdt.entries[0].access = 0;
	gdt.entries[0].granularity = 0;
	gdt.entries[0].base_hi = 0;

	// Kernel code 64
	gdt.entries[1].limit = 0;
	gdt.entries[1].base_low = 0;
	gdt.entries[1].base_mid = 0;
	gdt.entries[1].access = 0b10011010;
	gdt.entries[1].granularity = 0b00100000;
	gdt.entries[1].base_hi = 0;

	// Kernel data 64
	gdt.entries[2].limit = 0;
	gdt.entries[2].base_low = 0;
	gdt.entries[2].base_mid = 0;
	gdt.entries[2].access = 0b10010010;
	gdt.entries[2].granularity = 0;
	gdt.entries[2].base_hi = 0;

	// User data 64
	gdt.entries[3].limit = 0;
	gdt.entries[3].base_low = 0;
	gdt.entries[3].base_mid = 0;
	gdt.entries[3].access = 0b11110010;
	gdt.entries[3].granularity = 0;
	gdt.entries[3].base_hi = 0;

	// User code 64
	gdt.entries[4].limit = 0;
	gdt.entries[4].base_low = 0;
	gdt.entries[4].base_mid = 0;
	gdt.entries[4].access = 0b11111010;
	gdt.entries[4].granularity = 0b00100000;
	gdt.entries[4].base_hi = 0;

	// TSS
	gdt.tss.length = 104;
	gdt.tss.base_low = 0;
	gdt.tss.base_mid = 0;
	gdt.tss.flags1 = 0b10001001;
	gdt.tss.flags2 = 0;
	gdt.tss.base_hi = 0;
	gdt.tss.base_upper32 = 0;
	gdt.tss.reserved = 0;

	// Set the pointer
	gdt_pointer.limit = sizeof(gdt) - 1;
	gdt_pointer.ptr = (uint64_t)&gdt;

	gdt_reload();
	tss_reload();
}

void gdt_load_tss(size_t addr) {
	gdt.tss.base_low = (uint16_t)addr;
	gdt.tss.base_mid = (uint8_t)(addr >> 16);
	gdt.tss.flags1 = 0b10001001;
	gdt.tss.flags2 = 0;
	gdt.tss.base_hi = (uint8_t)(addr >> 24);
	gdt.tss.base_upper32 = (uint32_t)(addr >> 32);
	gdt.tss.reserved = 0;

	gdt_reload();
	tss_reload();
}
