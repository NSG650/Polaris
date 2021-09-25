/*
 * Copyright 2021 Misha
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

#include "madt.h"
#include "../kernel/panic.h"
#include "../klibc/asm.h"
#include "../klibc/printf.h"

DYNARRAY_GLOBAL(madt_local_apics);
DYNARRAY_GLOBAL(madt_io_apics);
DYNARRAY_GLOBAL(madt_isos);
DYNARRAY_GLOBAL(madt_nmis);

struct madt *madt;
//#define MADT_DEBUG

uintptr_t lapic_addr = 0;

uintptr_t acpi_get_lapic(void) {
	return lapic_addr;
}

void init_madt(void) {
	// Search for MADT table
	madt = acpi_find_sdt("APIC", 0);
	if (!madt) {
		PANIC("MADT table can't be found");
		__builtin_unreachable();
	}
	lapic_addr = madt->local_controller_addr;
	// Parse the MADT entries
	for (uint8_t *madt_ptr = (uint8_t *)madt->madt_entries_begin;
		 (uintptr_t)madt_ptr < (uintptr_t)madt + madt->sdt.length;
		 madt_ptr += *(madt_ptr + 1)) {
		switch (*(madt_ptr)) {
			case 0:
				// Processor local APIC
#ifdef MADT_DEBUG
				printf("ACPI/MADT: Found local APIC 0x%X\n",
					   madt_local_apics.length);
#endif
				DYNARRAY_PUSHBACK(madt_local_apics, (void *)madt_ptr);
				break;
			case 1:
				// I/O APIC
#ifdef MADT_DEBUG
				printf("ACPI/MADT: Found I/O APIC 0x%X\n",
					   madt_io_apics.length);
#endif
				DYNARRAY_PUSHBACK(madt_io_apics, (void *)madt_ptr);
				break;
			case 2:
				// Interrupt source override
#ifdef MADT_DEBUG
				printf("ACPI/MADT: Found ISO 0x%X\n", madt_isos.length);
#endif
				DYNARRAY_PUSHBACK(madt_isos, (void *)madt_ptr);
				break;
			case 4:
				// NMI
#ifdef MADT_DEBUG
				printf("ACPI/MADT: Found NMI 0x%X\n", madt_nmis.length);
#endif
				DYNARRAY_PUSHBACK(madt_nmis, (void *)madt_ptr);
				break;
			case 5:
				// Local APIC address override
				lapic_addr = QWORD_PTR(madt_ptr + 4);
				break;
		}
	}
}
