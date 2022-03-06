#include <fw/madt.h>
#include <io/mmio.h>
#include <mm/vmm.h>
#include <sys/apic.h>

static uint32_t ioapic_read(uintptr_t ioapic_address, size_t reg) {
	outmmdw((void *)ioapic_address + MEM_PHYS_OFFSET, reg & 0xFF);
	return inmmdw((void *)ioapic_address + MEM_PHYS_OFFSET + 16);
}

static void ioapic_write(uintptr_t ioapic_address, size_t reg, uint32_t data) {
	outmmdw((void *)ioapic_address + MEM_PHYS_OFFSET, reg & 0xFF);
	outmmdw((void *)ioapic_address + MEM_PHYS_OFFSET + 16, data);
}

static uint32_t get_gsi_count(uintptr_t ioapic_address) {
	return (ioapic_read(ioapic_address, 1) & 0xFF0000) >> 16;
}

static struct madt_ioapic *get_ioapic_by_gsi(uint32_t gsi) {
	for (int i = 0; i < madt_io_apics.length; i++) {
		struct madt_ioapic *ioapic = madt_io_apics.data[i];
		if (ioapic->gsib <= gsi &&
			ioapic->gsib + get_gsi_count(ioapic->addr) > gsi) {
			return ioapic;
		}
	}
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
