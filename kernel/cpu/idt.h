#ifndef IDT_H
#define IDT_H

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

#include <stdint.h>

#define KERNEL_CS 0x08

typedef struct {
	uint16_t low_offset;
	uint16_t sel;
	uint8_t always0;

	uint8_t flags;
	uint16_t mid_offset;
	uint32_t high_offset;
	uint32_t always0again;
} __attribute__((packed)) idt_gate_t;

typedef struct {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed)) idt_register_t;

#define IDT_ENTRIES 256

void set_idt_gate(int n, uint64_t handler);
void set_idt(void);

#endif
