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

#include "../klibc/printf.h"
#include "../mm/vmm.h"
#include "fadt.h"
#include "madt.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static bool use_xsdt;
static struct rsdt *rsdt;

// This function should look for all the ACPI tables and index them for later
// use
void acpi_init(struct rsdp *rsdp) {
	printf("ACPI: Revision: %d\n", rsdp->rev);

	if (rsdp->rev >= 2 && rsdp->xsdt_addr) {
		use_xsdt = true;
		rsdt = (struct rsdt *)((uintptr_t)rsdp->xsdt_addr + MEM_PHYS_OFFSET);
		printf("ACPI: Found XSDT at %X\n", (uintptr_t)rsdt);
	} else {
		use_xsdt = false;
		rsdt = (struct rsdt *)((uintptr_t)rsdp->rsdt_addr + MEM_PHYS_OFFSET);
		printf("ACPI: Found RSDT at %X\n", (uintptr_t)rsdt);
	}
	// Individual tables that need initialization
	init_fadt();
	init_madt();
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

// Find SDT by signature
void *acpi_find_sdt(const char *signature, int index) {
	int cnt = 0;

	size_t entries =
	  (rsdt->header.length - sizeof(struct sdt)) / (use_xsdt ? 8 : 4);

	for (size_t i = 0; i < entries; i++) {
		struct sdt *ptr;
		if (use_xsdt) {
			ptr = (struct sdt *)(uintptr_t)((uint64_t *)rsdt->ptrs_start)[i];
		} else {
			ptr = (struct sdt *)(uintptr_t)((uint32_t *)rsdt->ptrs_start)[i];
		}

		if (!memcmp(ptr->signature, signature, 4) &&
			!acpi_checksum(ptr, ptr->length) && cnt++ == index) {
			return (void *)ptr;
		}
	}

	printf("ACPI: \"%s\" not found\n", signature);
	return NULL;
}

void acpi_start(void) {
	acpi_enable();
}

void acpi_shutdown(void) {
	fadt_acpi_shutdown();
}

void acpi_reboot(void) {
	fadt_acpi_reboot();
}
