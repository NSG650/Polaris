/*
 * Copyright 2021 - 2023 Misha
 * Copyright 2021 - 2023 NSG650
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
#include <debug/debug.h>
#include <fw/madt.h>

struct madt *madt;

lapic_vec_t madt_local_apics;
ioapic_vec_t madt_io_apics;
iso_vec_t madt_isos;
nmi_vec_t madt_nmis;

uintptr_t lapic_addr = 0;

uintptr_t acpi_get_lapic(void) {
	return lapic_addr;
}

void madt_init(void) {
	vec_init(&madt_local_apics);
	vec_init(&madt_io_apics);
	vec_init(&madt_isos);
	vec_init(&madt_nmis);

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
				kprintf("MADT: Got local APIC 0x%x\n", madt_local_apics.length);
				vec_push(&madt_local_apics, (void *)madt_ptr);
				break;
			case 1:
				// I/O APIC
				kprintf("MADT: Got IO APIC 0x%x\n", madt_io_apics.length);
				vec_push(&madt_io_apics, (void *)madt_ptr);
				break;
			case 2:
				// Interrupt source override
				kprintf("MADT: Got ISO 0x%x\n", madt_isos.length);
				vec_push(&madt_isos, (void *)madt_ptr);
				break;
			case 4:
				// NMI
				kprintf("MADT: Got NMI 0x%x\n", madt_nmis.length);
				vec_push(&madt_nmis, (void *)madt_ptr);
				break;
			case 5:
				// Local APIC address override
				lapic_addr = QWORD_PTR(madt_ptr + 4);
				break;
		}
	}
}
