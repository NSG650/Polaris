#include <asm/asm.h>
#include <debug/debug.h>
#include <fw/madt.h>

struct madt *madt;

struct madt_lapic **madt_local_apics;
struct madt_ioapic **madt_io_apics;
struct madt_iso **madt_isos;
struct madt_nmi **madt_nmis;

uintptr_t lapic_addr = 0;

uintptr_t acpi_get_lapic(void) {
	return lapic_addr;
}

void madt_init(void) {
	madt_local_apics = vector_create();
	madt_io_apics = vector_create();
	madt_isos = vector_create();
	madt_nmis = vector_create();

	madt = acpi_find_sdt("APIC", 0);
	if (!madt)
		panic("Unable to find MADT table\n");
	lapic_addr = madt->local_controller_addr;
	kprintf("MADT: MADT at %p\n", madt);
	for (uint8_t *madt_ptr = (uint8_t *)madt->madt_entries_begin;
		 (uintptr_t)madt_ptr < (uintptr_t)madt + madt->sdt.length;
		 madt_ptr += *(madt_ptr + 1)) {
		switch (*(madt_ptr)) {
			case 0:
				// Processor local APIC
				kprintf("MADT: Got local APIC 0x%x\n",
						vector_size(madt_local_apics));
				vector_add(&madt_local_apics, (void *)madt_ptr);
				break;
			case 1:
				// I/O APIC
				kprintf("MADT: Got IO APIC 0x%x\n", vector_size(madt_io_apics));
				vector_add(&madt_io_apics, (void *)madt_ptr);
				break;
			case 2:
				// Interrupt source override
				kprintf("MADT: Got ISO 0x%x\n", vector_size(madt_isos));
				vector_add(&madt_isos, (void *)madt_ptr);
				break;
			case 4:
				// NMI
				kprintf("MADT: Got NMI 0x%x\n", vector_size(madt_nmis));
				vector_add(&madt_nmis, (void *)madt_ptr);
				break;
			case 5:
				// Local APIC address override
				lapic_addr = QWORD_PTR(madt_ptr + 4);
				break;
		}
	}
}
