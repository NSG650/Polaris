/*
 * Copyright 2021, 2022 Misha
 * Copyright 2021, 2022 NSG650
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

#include "madt.h"
#include "../kernel/panic.h"
#include "../klibc/asm.h"
#include "../klibc/printf.h"

lapic_vec_t madt_local_apics;
ioapic_vec_t madt_io_apics;
iso_vec_t madt_isos;
nmi_vec_t madt_nmis;

struct madt *madt;

uintptr_t lapic_addr = 0;

uintptr_t acpi_get_lapic(void) {
	return lapic_addr;
}

void init_madt(void) {
	// Initialize dynamic arrays
	vec_init(&madt_local_apics);
	vec_init(&madt_io_apics);
	vec_init(&madt_isos);
	vec_init(&madt_nmis);
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
				printf("ACPI/MADT: Found local APIC 0x%X\n",
					   madt_local_apics.length);
				vec_push(&madt_local_apics, (void *)madt_ptr);
				break;
			case 1:
				// I/O APIC
				printf("ACPI/MADT: Found I/O APIC 0x%X\n",
					   madt_io_apics.length);
				vec_push(&madt_io_apics, (void *)madt_ptr);
				break;
			case 2:
				// Interrupt source override
				printf("ACPI/MADT: Found ISO 0x%X\n", madt_isos.length);
				vec_push(&madt_isos, (void *)madt_ptr);
				break;
			case 4:
				// NMI
				printf("ACPI/MADT: Found NMI 0x%X\n", madt_nmis.length);
				vec_push(&madt_nmis, (void *)madt_ptr);
				break;
			case 5:
				// Local APIC address override
				lapic_addr = QWORD_PTR(madt_ptr + 4);
				break;
		}
	}
}
