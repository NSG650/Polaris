/*
 * Copyright 2021, 2022 Sebastian
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
#include "../cpu/cpu.h"
#include "../mm/vmm.h"
#include "../sched/process.h"
#include "../sched/scheduler.h"
#include "../sys/hpet.h"
#include "../sys/mmio.h"
#include "cpu.h"
#include "isr.h"
#include <cpuid.h>
#include <lai/helpers/pm.h>
#include <lai/helpers/sci.h>

static uintptr_t lapic_addr = 0;
static bool x2apic = false;

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

static uint32_t lapic_read(uint32_t reg) {
	if (x2apic) {
		return rdmsr(reg_to_x2apic(reg));
	}
	return mmind((void *)lapic_addr + MEM_PHYS_OFFSET + reg);
}

static void lapic_write(uint32_t reg, uint32_t value) {
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

void lapic_init(uint8_t processor_id) {
	// Enable APIC and x2APIC if available
	uint64_t apic_msr = rdmsr(0x1B);
	// Set enable flag
	apic_msr |= 1 << 11;
	uint32_t a = 0, b = 0, c = 0, d = 0;
	if (__get_cpuid(1, &a, &b, &c, &d)) {
		if (c & CPUID_X2APIC) {
			x2apic = true;
			// Set x2APIC flag
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
	// Set NMIs
	for (int i = 0; i < madt_nmis.length; i++) {
		struct madt_nmi *nmi = madt_nmis.data[i];
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

void apic_timer_init(void) {
	lapic_write(0x3E0, 3);			// Divide by 16
	lapic_write(0x380, 0xFFFFFFFF); // Set value to -1

	hpet_usleep(10000);
	lapic_write(0x320, 0x10000);

	uint32_t tick_in_10ms = 0xFFFFFFFF - lapic_read(0x390);

	// Set to receive interrupt in vector 32 (IRQ 0)
	lapic_write(0x320, 32 | 0x20000);
	lapic_write(0x3E0, 3);
	lapic_write(0x380, tick_in_10ms / 10);
}

void sci_interrupt(registers_t *reg) {
	(void)reg;
	uint16_t ev = lai_get_sci_event();

	if (ev & ACPI_POWER_BUTTON) {
		lai_enter_sleep(5);
	}
}

size_t timer_tick = 0;

void timer_interrupt(registers_t *reg) {
	(void)reg;
	timer_tick++;
	for (int i = 0; i < ptable.length; i++) {
		struct process *proc = ptable.data[i];
		if (proc->state == BLOCKED && proc->block_on == ON_SLEEP &&
			timer_tick >= proc->target_tick) {
			proc->target_tick = 0;
			process_unblock(proc);
		}
	}

	struct process *proc = running_proc();
	struct thread *thrd = running_thrd();
	if (proc != NULL && proc->state == RUNNING && thrd != NULL &&
		thrd->state_t == RUNNING) {
		proc->state = READY;
		thrd->state_t = READY;
		yield_to_scheduler();
	}
}

void apic_init(void) {
	lapic_addr = acpi_get_lapic();
	lapic_init(madt_local_apics.data[0]->processor_id);
	// Register SCI interrupt
	acpi_fadt_t *facp = acpi_find_sdt("FACP", 0);
	// We remap interrupts to IRQ + 48
	ioapic_redirect_irq(facp->sci_irq, facp->sci_irq + 48);
	isr_register_handler(facp->sci_irq + 48, sci_interrupt);
	// Initialize memory to avoid possible page faults
	vec_init(&ptable);
	memset(cpu_locals, 0, sizeof(struct cpu_local));
	memset(this_cpu->cpu_state, 0, sizeof(struct cpu_state));
	// Timer
	ioapic_redirect_irq(0, 48);
	isr_register_handler(48, timer_interrupt);
	apic_timer_init();
}
