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

idt_gate_t idt[256] = {0};

void set_idt_gate(int n, void *handler) {
	uint64_t p = (uint64_t)handler;

	idt[n].offset_lo = (uint16_t)p;
	idt[n].selector = 0x08;
	idt[n].ist = 0;
	idt[n].flags = 0x8E;
	idt[n].offset_mid = (uint16_t)(p >> 16);
	idt[n].offset_hi = (uint32_t)(p >> 32);
	idt[n].zero = 0;
}

void set_idt(void) {
	idt_register_t idt_ptr = {sizeof(idt) - 1, (uint64_t)idt};

	asm volatile("lidtq %0" : : "m"(idt_ptr));
}
