#include <cpu/msr.h>
#include <cpu_features.h>
#include <cpuid.h>
#include <debug/debug.h>
#include <fw/madt.h>
#include <io/mmio.h>
#include <mm/vmm.h>
#include <reg.h>
#include <sys/apic.h>
#include <sys/isr.h>
#include <sys/timer.h>

static uintptr_t lapic_addr = 0;
static bool x2apic = false;
uint32_t tick_in_10ms = 0;

// Converts xAPIC MMIO offset into x2APIC MSR
static uint32_t reg_to_x2apic(uint32_t reg) {
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

void lapic_init(uint8_t cpu_id) {
	if (lapic_addr == 0) {
		lapic_addr = acpi_get_lapic();
	}
	kprintf("LAPIC: Setting LAPIC on CPU%d\n", cpu_id);
	// Enable APIC and x2APIC if available
	uint64_t apic_msr = rdmsr(0x1B);
	// Set enable flag
	apic_msr |= 1 << 11;
	uint32_t a = 0, b = 0, c = 0, d = 0;
	__get_cpuid(1, &a, &b, &c, &d);
	if (c & CPUID_X2APIC) {
		x2apic = true;
        kprintf("LAPIC: Using x2APIC\n");
		// Set x2APIC flag
		apic_msr |= 1 << 10;
	}
	wrmsr(0x1B, apic_msr);
	// Initialize local APIC
	lapic_write(0x80, 0);
	lapic_write(0xF0, lapic_read(0xF0) | 0x100);
	if (!x2apic) {
		lapic_write(0xE0, 0xF0000000);
		lapic_write(0xD0, lapic_read(0x20));
	}
	for (int i = 0; i < madt_nmis.length; i++) {
		struct madt_nmi *nmi = madt_nmis.data[i];
		lapic_set_nmi(2, cpu_id, nmi->processor, nmi->flags, nmi->lint);
	}
	if (!tick_in_10ms) {
		lapic_write(0x3E0, 3);			// Divide by 16
		lapic_write(0x380, 0xFFFFFFFF); // Set value to -1
		timer_sleep(10);
		lapic_write(0x320, 0x10000);
		tick_in_10ms = 0xFFFFFFFF - lapic_read(0x390);
	}
}

uint8_t lapic_get_id(void) {
	return (uint8_t)(lapic_read(0x20) >> 24);
}

void apic_eoi(void) {
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

void apic_send_ipi(uint8_t lapic_id, uint8_t vector) {
	if (x2apic) {
		// Write MSR directly, because lapic_write receives a 32-bit argument
		// Whilst in x2APIC, 0x830 is a 64-bit register
		wrmsr(0x830, ((uint64_t)lapic_id << 32) | vector);
	} else {
		lapic_write(0x310, (lapic_id << 24));
		lapic_write(0x300, vector);
	}
}
