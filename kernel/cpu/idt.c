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

#include "idt.h"

idt_gate_t idt[IDT_ENTRIES];
idt_register_t idt_reg;

void set_idt_gate(int n, uint64_t handler) {
	idt[n].low_offset = (uint16_t)(handler >> 0);
	idt[n].sel = KERNEL_CS;
	idt[n].always0 = 0;
	idt[n].flags = 0x8E;
	idt[n].mid_offset = (uint16_t)(handler >> 16);
	idt[n].high_offset = (uint32_t)(handler >> 32);
	idt[n].always0again = 0;
}

void set_idt(void) {
	idt_reg.base = (uint64_t)&idt;
	idt_reg.limit = IDT_ENTRIES * sizeof(idt_gate_t) - 1;
	asm volatile("lidtq %0" : "=m"(idt_reg));
}
