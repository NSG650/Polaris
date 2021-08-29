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

typedef struct {
	uint16_t offset_lo;
	uint16_t selector;
	uint8_t ist;
	uint8_t flags;
	uint16_t offset_mid;
	uint32_t offset_hi;
	uint32_t zero;
} __attribute__((packed)) idt_gate_t;

typedef struct {
	uint16_t size;
	uint64_t address;
} __attribute__((packed)) idt_register_t;

void set_idt_gate(int n, void *handler);
void set_idt(void);

#endif
