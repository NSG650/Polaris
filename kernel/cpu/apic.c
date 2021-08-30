/*
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

#include "apic.h"
#include "../acpi/madt.h"
#include "../klibc/printf.h"
#include "../mm/vmm.h"
#include "../sys/hpet.h"
#include <stdint.h>

static uintptr_t lapic_addr = 0;

static uint32_t lapic_read(uint32_t reg) {
	return *((volatile uint32_t *)(lapic_addr + MEM_PHYS_OFFSET + reg));
}

static void lapic_write(uint32_t reg, uint32_t value) {
	*((volatile uint32_t *)(lapic_addr + MEM_PHYS_OFFSET + reg)) = value;
}

void lapic_enable_spurious(void) {
	lapic_write(0xF0, lapic_read(0xF0) | 0x100);
}

typeof(madt_io_apics) ioapics;

static uint32_t ioapic_read(uintptr_t ioapic_address, size_t reg) {
	*((volatile uint32_t *)(ioapic_address + MEM_PHYS_OFFSET)) = reg;
	return *((volatile uint32_t *)(ioapic_address + MEM_PHYS_OFFSET + 16));
}

static void ioapic_write(uintptr_t ioapic_address, size_t reg, uint32_t data) {
	*((volatile uint32_t *)(ioapic_address + MEM_PHYS_OFFSET)) = reg;
	*((volatile uint32_t *)(ioapic_address + MEM_PHYS_OFFSET + 16)) = data;
}

static uint32_t get_gsi_count(uintptr_t ioapic_address) {
	return (ioapic_read(ioapic_address, 0x1) & 0xFF0000) >> 16;
}

static struct madt_ioapic *get_ioapic_by_gsi(uint32_t gsi) {
	for (size_t i = 0; i < ioapics.length; i++) {
		struct madt_ioapic *ioapic = ioapics.storage[i];
		if (ioapic->gsib <= gsi &&
			ioapic->gsib + get_gsi_count(ioapic->addr + MEM_PHYS_OFFSET) >
			  gsi) {
			return ioapic;
		}
	}

	return NULL;
}

void ioapic_init(void) {
	ioapics = madt_io_apics;
}

void ioapic_redirect_gsi(uint8_t lapic_id, uint32_t gsi, uint8_t vec,
						 uint16_t flags, bool status) {
	size_t io_apic = get_ioapic_by_gsi(gsi)->apic_id;

	uint64_t redirect = vec;

	// Active high(0) or low(1)
	if (flags & 2) {
		redirect |= (1 << 13);
	}

	// Edge(0) or level(1) triggered
	if (flags & 8) {
		redirect |= (1 << 15);
	}

	if (!status) {
		// Set mask bit
		redirect |= (1 << 16);
	}

	// Set target APIC ID
	redirect |= ((uint64_t)lapic_id) << 56;
	uint32_t ioredtbl = (gsi - ioapics.storage[io_apic]->gsib) * 2 + 16;

	ioapic_write(io_apic, ioredtbl + 0, (uint32_t)redirect);
	ioapic_write(io_apic, ioredtbl + 1, (uint32_t)(redirect >> 32));
}

void ioapic_redirect_irq(uint8_t lapic_id, uint8_t irq, uint8_t vect,
						 bool status) {
	typeof(madt_isos) isos = madt_isos;
	for (size_t i = 0; i < isos.length; i++) {
		if (isos.storage[i]->irq_source == irq) {
			ioapic_redirect_gsi(lapic_id, vect, isos.storage[i]->gsi,
								isos.storage[i]->flags, status);
			return;
		}
	}

	ioapic_redirect_gsi(lapic_id, vect, irq, 0, status);
}

void apic_send_ipi(uint8_t lapic_id, uint8_t vector) {
	lapic_write(0x310, ((uint32_t)lapic_id) << 24);
	lapic_write(0x300, vector);
}

void apic_eoi(void) {
	lapic_write(0x0B0, 0);
}

void apic_timer_init(void) {
	lapic_addr = acpi_get_lapic();

	lapic_write(0x3E0, 0x3);
	lapic_write(0x380, 0xFFFFFFFF);

	hpet_usleep(10000);
	lapic_write(0x320, 0x10000);

	uint32_t tickIn10ms = 0xFFFFFFFF - lapic_read(0x390);

	lapic_write(0x320, 32 | 0x20000);
	lapic_write(0x3E0, 0x3);
	lapic_write(0x380, tickIn10ms);
}

void apic_init(void) {
	lapic_addr = acpi_get_lapic();
	lapic_enable_spurious();
	ioapic_init();
}
