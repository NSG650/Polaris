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

#include <debug/debug.h>
#include <fw/acpi.h>
#include <fw/madt.h>
#include <io/pci.h>
#include <klibc/mem.h>
#include <lai/core.h>
#include <lai/helpers/sci.h>
#include <mm/vmm.h>
#include <stdbool.h>
#include <sys/hpet.h>

bool use_xsdt;
struct rsdt *rsdt;
uint8_t revision;

void acpi_init(acpi_xsdp_t *rsdp) {
	kprintf("ACPI: Revision: %u\n", rsdp->revision);
	revision = rsdp->revision;
	if (rsdp->revision >= 2 && rsdp->xsdt) {
		use_xsdt = true;
		rsdt = (struct rsdt *)((uintptr_t)rsdp->xsdt + MEM_PHYS_OFFSET);
		kprintf("ACPI: Using XSDT at %p\n", (uintptr_t)rsdt);
	} else {
		use_xsdt = false;
		rsdt = (struct rsdt *)((uintptr_t)rsdp->rsdt + MEM_PHYS_OFFSET);
		kprintf("ACPI: Using RSDT at %p\n", (uintptr_t)rsdt);
	}
	hpet_init();
	pci_init();
	lai_set_acpi_revision(revision);
	lai_create_namespace();
	lai_enable_acpi(1);
	madt_init();
}

// Following function based on
// https://github.com/managarm/lai/blob/master/helpers/pc-bios.c's function
// lai_bios_calc_checksum()
static uint8_t acpi_checksum(void *ptr, size_t size) {
	uint8_t sum = 0, *_ptr = ptr;
	for (size_t i = 0; i < size; i++)
		sum += _ptr[i];
	return sum;
}

void *acpi_find_sdt(const char *signature, int index) {
	int cnt = 0;

	const size_t entries =
		(rsdt->header.length - sizeof(acpi_header_t)) / (use_xsdt ? 8 : 4);

	for (size_t i = 0; i < entries; i++) {
		acpi_header_t *ptr;
		if (use_xsdt) {
			ptr = (acpi_header_t *)(uintptr_t)((uint64_t *)rsdt->ptrs_start)[i];
		} else {
			ptr = (acpi_header_t *)(uintptr_t)((uint32_t *)rsdt->ptrs_start)[i];
		}

		if (!memcmp(ptr->signature, signature, 4) &&
			!acpi_checksum(ptr, ptr->length) && cnt++ == index) {
			return (void *)ptr + MEM_PHYS_OFFSET;
		}
	}

	return NULL;
}
