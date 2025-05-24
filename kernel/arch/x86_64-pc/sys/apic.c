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

#include <asm/asm.h>
#include <cpu/msr.h>
#include <cpu_features.h>
#include <cpuid.h>
#include <debug/debug.h>
#include <fw/acpi.h>
#include <fw/madt.h>
#include <io/mmio.h>
#include <klibc/time.h>
#include <mm/vmm.h>
#include <sys/apic.h>
#include <sys/hpet.h>
#include <sys/isr.h>
#include <sys/pic.h>
#include <sys/prcb.h>
#include <sys/timer.h>

static uintptr_t lapic_addr = 0;
static bool x2apic = false;
uint32_t tick_in_10ms = 0;

// Converts xAPIC MMIO offset into x2APIC MSR
static inline uint32_t reg_to_x2apic(uint32_t reg) {
	uint32_t x2apic_reg = 0;
	// MSR 831H is reserved; read/write operations cause general-protection
	// exceptions. The contents of the APIC register at MMIO offset 310H are
	// accessible in x2APIC mode through the MSR at address 830H
	// -- Intel SDM Volume 3A 10.12.1.2 Note 4
	if (reg == 0x310) {
		x2apic_reg = 0x30;
	} else {
		x2apic_reg = reg >> 4;
	}
	return x2apic_reg + 0x800;
}

uint32_t lapic_read(uint32_t reg) {
	if (x2apic) {
		return rdmsr(reg_to_x2apic(reg));
	}
	return mmind((void *)lapic_addr + MEM_PHYS_OFFSET + reg);
}

void lapic_write(uint32_t reg, uint32_t value) {
	if (x2apic) {
		wrmsr(reg_to_x2apic(reg), value);
	} else {
		mmoutd((void *)lapic_addr + MEM_PHYS_OFFSET + reg, value);
	}
}

static void lapic_set_nmi(uint8_t vec, uint8_t current_processor_id,
						  uint8_t processor_id, uint16_t flags, uint8_t lint) {
	// A value of 0xFF means all the processors
	if (processor_id != 0xFF) {
		if (current_processor_id != processor_id) {
			return;
		}
	}

	// Set to raise in vector number "vec" and set NMI flag
	uint32_t nmi = 0x400 | vec;

	// Set to active low if needed
	if (flags & 2) {
		nmi |= 1 << 13;
	}

	// Set to level triggered if needed
	if (flags & 8) {
		nmi |= 1 << 15;
	}

	// Use the proper LINT register
	if (lint == 0) {
		lapic_write(0x350, nmi);
	} else if (lint == 1) {
		lapic_write(0x360, nmi);
	}
}

uint8_t lapic_get_id(void) {
	return (uint8_t)(lapic_read(0x20) >> 24);
}

void lapic_init(uint8_t processor_id) {
	kprintf("LAPIC: Setting up LAPIC on Processor %u\n", processor_id);
	uint64_t apic_msr = rdmsr(0x1B);
	// Set APIC enable flag
	apic_msr |= 1 << 11;
	uint32_t a = 0, b = 0, c = 0, d = 0;
	if (__get_cpuid(1, &a, &b, &c, &d)) {
		if (c & CPUID_X2APIC) {
			x2apic = true;
			// Set x2APIC flag if support is detected
			apic_msr |= 1 << 10;
		}
	}
	wrmsr(0x1B, apic_msr);

	// Initialize local APIC
	lapic_write(0x80, 0);
	lapic_write(0xF0, lapic_read(0xF0) | 0x100);
	if (!x2apic) {
		lapic_write(0xE0, 0xF0000000);
		lapic_write(0xD0, lapic_read(0x20));
	}

	// Set NMIs according to the MADT
	for (int i = 0; i < madt_nmis.length; i++) {
		struct madt_nmi *nmi = madt_nmis.data[i];
		lapic_set_nmi(2, processor_id, nmi->processor, nmi->flags, nmi->lint);
	}

	// Set up APIC timer

	// Tell APIC timer to divide by 16
	lapic_write(0x3E0, 3);
	// Set timer init counter to -1
	lapic_write(0x380, 0xFFFFFFFF);

	timer_sleep(10);

	// Stop the APIC timer
	lapic_write(0x320, 0x10000);

	// How much the APIC timer ticked in 10ms
	tick_in_10ms = 0xFFFFFFFF - lapic_read(0x390);

	// Start timer as periodic on IRQ 0
	lapic_write(0x320, 32 | 0x20000);
	// With divider 16
	lapic_write(0x3E0, 3);
	lapic_write(0x380, tick_in_10ms / 10);
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
	// Search through every I/O APIC to find its GSI
	for (int i = 0; i < madt_io_apics.length; i++) {
		struct madt_ioapic *ioapic = madt_io_apics.data[i];
		if (ioapic->gsib <= gsi &&
			ioapic->gsib + get_gsi_count(ioapic->addr) > gsi) {
			return ioapic;
		}
	}

	// Return NULL if none was found
	return NULL;
}

void ioapic_redirect_gsi(uint32_t gsi, uint8_t vec, uint16_t flags) {
	// Get I/O APIC address of the GSI
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
	// Use ISO table to find flags and interrupt overrides
	for (int i = 0; i < madt_isos.length; i++) {
		if (madt_isos.data[i]->irq_source == irq) {
			ioapic_redirect_gsi(madt_isos.data[i]->gsi, vect,
								madt_isos.data[i]->flags);
			return;
		}
	}

	ioapic_redirect_gsi(irq, vect, 0);
}

void apic_send_ipi(uint32_t lapic_id, uint32_t flags) {
	if (x2apic) {
		// Write MSR directly, because lapic_write receives a 32-bit argument
		// Whilst in x2APIC, 0x830 is a 64-bit register
		wrmsr(0x830, ((uint64_t)lapic_id << 32) | flags);
	} else {
		lapic_write(0x310, (lapic_id << 24));
		lapic_write(0x300, flags);
	}
}

void apic_eoi(void) {
	// Writing any other value different than 0 may cause a #GP exception
	lapic_write(0xB0, 0);
}

void timer_stop_sched(void) {
	lapic_write(0x380, 0);
	lapic_write(0x320, (1 << 16));
}

void timer_sched_oneshot(uint8_t isr, uint32_t us) {
	timer_stop_sched();
	lapic_write(0x320, isr | 0x20000);
	lapic_write(0x3E0, 3);
	lapic_write(0x380, ((tick_in_10ms * (us / 1000))) / 10);
}

void apic_init(void) {
	pic_init();

	lapic_addr = acpi_get_lapic();
	lapic_init(madt_local_apics.data[0]->processor_id);
	ioapic_redirect_irq(0, 48);
}
