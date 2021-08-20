#include "madt.h"
#include "../kernel/panic.h"
#include "../klibc/printf.h"

struct madt *madt;

DYNARRAY_GLOBAL(madt_local_apics);
DYNARRAY_GLOBAL(madt_io_apics);
DYNARRAY_GLOBAL(madt_isos);
DYNARRAY_GLOBAL(madt_nmis);

void init_madt(void) {
	// Search for MADT table
	madt = acpi_find_sdt("APIC", 0);
	if (!madt) {
		PANIC("MADT table can't be found");
		return;
	}
	// Parse the MADT entries
	for (uint8_t *madt_ptr = (uint8_t *)madt->madt_entries_begin;
		 (uintptr_t)madt_ptr < (uintptr_t)madt + madt->sdt.length;
		 madt_ptr += *(madt_ptr + 1)) {
		switch (*(madt_ptr)) {
			case 0:
				// Processor local APIC
				printf("ACPI/MADT: Found local APIC 0x%X\n",
					   madt_local_apics.length);
				DYNARRAY_PUSHBACK(madt_local_apics, (void *)madt_ptr);
				break;
			case 1:
				// I/O APIC
				printf("ACPI/MADT: Found I/O APIC 0x%X\n",
					   madt_io_apics.length);
				DYNARRAY_PUSHBACK(madt_io_apics, (void *)madt_ptr);
				break;
			case 2:
				// Interrupt Source Override
				printf("ACPI/MADT: Found ISO 0x%X\n", madt_isos.length);
				DYNARRAY_PUSHBACK(madt_isos, (void *)madt_ptr);
				break;
			case 4:
				// NMI
				printf("ACPI/MADT: Found NMI 0x%X\n", madt_nmis.length);
				DYNARRAY_PUSHBACK(madt_nmis, (void *)madt_ptr);
				break;
		}
	}
}
