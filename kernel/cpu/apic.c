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
#include "../mm/vmm.h"
#include "../sys/mmio.h"
#include "isr.h"
#include <lai/helpers/pm.h>
#include <lai/helpers/sci.h>

static uintptr_t lapic_addr = 0;

static uint32_t lapic_read(uint32_t reg) {
	return mmind((void *)lapic_addr + MEM_PHYS_OFFSET + reg);
}

static void lapic_write(uint32_t reg, uint32_t value) {
	mmoutd((void *)lapic_addr + MEM_PHYS_OFFSET + reg, value);
}

static void lapic_set_nmi(uint8_t vec, uint8_t current_processor_id,
						  uint8_t processor_id, uint16_t flags, uint8_t lint) {
	if (processor_id != 0xFF) {
		if (current_processor_id != processor_id) {
			return;
		}
	}

	uint32_t nmi = 0x800 | vec;

	if (flags & 2) {
		nmi |= 1 << 13;
	}

	if (flags & 8) {
		nmi |= 1 << 15;
	}

	if (lint == 0) {
		lapic_write(0x350, nmi);
	} else if (lint == 1) {
		lapic_write(0x360, nmi);
	}
}

void lapic_init(uint8_t processor_id) {
	lapic_write(0x80, 0);
	lapic_write(0xF0, lapic_read(0xF0) | 0x100);
	lapic_write(0xE0, 0xFFFFFFFF);
	lapic_write(0xD0, 0x1000000);
	for (size_t i = 0; i < madt_nmis.length; i++) {
		struct madt_nmi *nmi = madt_nmis.storage[i];
		lapic_set_nmi(2, processor_id, nmi->processor, nmi->flags, nmi->lint);
	}
}

static uint32_t ioapic_read(uintptr_t ioapic_address, size_t reg) {
	mmoutd((void *)ioapic_address + MEM_PHYS_OFFSET, reg & 0xFF);
	return mmind((void *)ioapic_address + MEM_PHYS_OFFSET + 16);
}

static void ioapic_write(uintptr_t ioapic_address, size_t reg, uint32_t data) {
	mmoutd((void *)ioapic_address + MEM_PHYS_OFFSET, reg & 0xFF);
	mmoutd((void *)ioapic_address + MEM_PHYS_OFFSET + 16, data);
}

static uint32_t get_gsi_count(uintptr_t ioapic_address) {
	return (ioapic_read(ioapic_address, 1) & 0xFF0000) >> 16;
}

static struct madt_ioapic *get_ioapic_by_gsi(uint32_t gsi) {
	for (size_t i = 0; i < madt_io_apics.length; i++) {
		struct madt_ioapic *ioapic = madt_io_apics.storage[i];
		if (ioapic->gsib <= gsi &&
			ioapic->gsib + get_gsi_count(ioapic->addr) > gsi) {
			return ioapic;
		}
	}

	return NULL;
}

void ioapic_redirect_gsi(uint32_t gsi, uint8_t vec, uint16_t flags) {
	size_t io_apic = get_ioapic_by_gsi(gsi)->addr;

	uint32_t low_index = 0x10 + (gsi - get_ioapic_by_gsi(gsi)->gsib) * 2;
	uint32_t high_index = low_index + 1;

	uint32_t high = ioapic_read(io_apic, high_index);

	// Set APIC ID
	high &= ~0xFF000000;
	high |= ioapic_read(io_apic, 0) << 24;
	ioapic_write(io_apic, high_index, high);

	uint32_t low = ioapic_read(io_apic, low_index);

	// Unmask the IRQ
	low &= ~(1 << 16);

	// Set to physical delivery mode
	low &= ~(1 << 11);

	// Set to fixed delivery mode
	low &= ~0x700;

	// Set delivery vector
	low &= ~0xFF;
	low |= vec;

	// Active high(0) or low(1)
	if (flags & 2) {
		low |= 1 << 13;
	}

	// Edge(0) or level(1) triggered
	if (flags & 8) {
		low |= 1 << 15;
	}

	ioapic_write(io_apic, low_index, low);
}

void ioapic_redirect_irq(uint32_t irq, uint8_t vect) {
	for (size_t i = 0; i < madt_isos.length; i++) {
		if (madt_isos.storage[i]->irq_source == irq) {
			ioapic_redirect_gsi(madt_isos.storage[i]->gsi, vect,
								madt_isos.storage[i]->flags);
			return;
		}
	}

	ioapic_redirect_gsi(irq, vect, 0);
}

void apic_send_ipi(uint8_t lapic_id, uint8_t vector) {
	lapic_write(0x310, ((uint32_t)lapic_id) << 24);
	lapic_write(0x300, vector);
}

void apic_eoi(void) {
	lapic_write(0x0B0, 0);
}

void sci_interrupt(registers_t *reg) {
	(void)reg;
	uint16_t ev = lai_get_sci_event();

	if (ev & ACPI_POWER_BUTTON) {
		lai_enter_sleep(5);
	}
}

void apic_init(void) {
	lapic_addr = acpi_get_lapic();
	lapic_init(madt_local_apics.storage[0]->processor_id);
	// Register SCI interrupt
	acpi_fadt_t *facp = (acpi_fadt_t *)acpi_find_sdt("FACP", 0);
	ioapic_redirect_irq(facp->sci_irq, 73);
	isr_register_handler(73, sci_interrupt);
}
