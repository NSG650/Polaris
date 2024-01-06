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

#include <klibc/mem.h>
#include <locks/spinlock.h>
#include <sys/gdt.h>

struct gdt_desc {
	uint16_t limit;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t access;
	uint8_t granularity;
	uint8_t base_hi;
} __attribute__((packed));

struct tss_desc {
	uint16_t length;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t flags1;
	uint8_t flags2;
	uint8_t base_hi;
	uint32_t base_upper32;
	uint32_t reserved;
} __attribute__((packed));

struct gdt_ptr {
	uint16_t limit;
	uint64_t ptr;
} __attribute__((packed));

struct gdtr {
	struct gdt_desc entries[5];
	struct tss_desc tss;
} __attribute__((packed));

struct gdtr gdt = {0};
struct gdt_ptr gdt_pointer = {0};
lock_t gdt_lock;

extern void gdt_reload(void);
extern void tss_reload(void);

void gdt_init(void) {
	spinlock_acquire_or_wait(&gdt_lock);
	// Kernel code
	gdt.entries[1].access = 0b10011010;
	gdt.entries[1].granularity = 0b00100000;

	// Kernel data
	gdt.entries[2].access = 0b10010010;

	// User data
	gdt.entries[3].access = 0b11110010;

	// User code
	gdt.entries[4].access = 0b11111010;
	gdt.entries[4].granularity = 0b00100000;

	// TSS
	gdt.tss.length = sizeof(struct tss);
	gdt.tss.flags1 = 0b10001001;

	// Set the pointer
	gdt_pointer.limit = sizeof(gdt) - 1;
	gdt_pointer.ptr = (uint64_t)&gdt;

	gdt_reload();
	tss_reload();
	spinlock_drop(&gdt_lock);
}

void gdt_load_tss(size_t addr) {
	spinlock_acquire_or_wait(&gdt_lock);
	gdt.tss.base_low = (uint16_t)addr;
	gdt.tss.base_mid = (uint8_t)(addr >> 16);
	gdt.tss.flags1 = 0b10001001;
	gdt.tss.base_hi = (uint8_t)(addr >> 24);
	gdt.tss.base_upper32 = (uint32_t)(addr >> 32);

	tss_reload();
	spinlock_drop(&gdt_lock);
}
