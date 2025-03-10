#ifndef APIC_H
#define APIC_H

/*
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

#include <reg.h>
#include <stddef.h>
#include <stdint.h>

void apic_eoi(void);
void apic_init(void);
void apic_send_ipi(uint32_t lapic_id, uint32_t flags);
void ioapic_redirect_irq(uint32_t irq, uint8_t vect);
uint32_t lapic_read(uint32_t reg);
void lapic_write(uint32_t reg, uint32_t value);
uint8_t lapic_get_id(void);
void lapic_init(uint8_t processor_id);
void timer_sched_oneshot(uint8_t isr, uint32_t us);

#endif
