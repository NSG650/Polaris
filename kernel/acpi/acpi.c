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

#include "../cpu/ports.h"
#include "../kernel/panic.h"
#include "../klibc/liballoc.h"
#include "../klibc/printf.h"
#include "../mm/vmm.h"
#include "../sys/pci.h"
#include "madt.h"
#include <lai/core.h>
#include <lai/helpers/sci.h>
#include <lai/host.h>
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
	lai_set_acpi_revision(rsdp->rev);
	lai_create_namespace();
	lai_enable_acpi(0);
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

void laihost_log(int level, const char *msg) {
	switch (level) {
		case LAI_DEBUG_LOG:
			printf("ACPI: Debug: %s\n", msg);
			break;
		case LAI_WARN_LOG:
			printf("ACPI: Warning: %s\n", msg);
			break;
		default:
			printf("ACPI: %s\n", msg);
			break;
	}
}

__attribute__((noreturn)) void laihost_panic(const char *msg) {
	PANIC(msg);
	__builtin_unreachable();
}

void *laihost_malloc(size_t size) {
	return (void *)(uintptr_t)kmalloc(size);
}

void laihost_free(void *ptr, size_t) {
	kfree(ptr);
}

void *laihost_realloc(void *ptr, size_t newsize, size_t) {
	return (void *)(uintptr_t)krealloc(ptr, newsize);
}

void *laihost_map(size_t address, size_t) {
	return (void *)address + MEM_PHYS_OFFSET;
}

void laihost_outb(uint16_t port, uint8_t val) {
	port_byte_out(port, val);
}

void laihost_outw(uint16_t port, uint16_t val) {
	port_word_out(port, val);
}

void laihost_outd(uint16_t port, uint32_t val) {
	port_dword_out(port, val);
}

uint8_t laihost_inb(uint16_t port) {
	return port_byte_in(port);
}

uint16_t laihost_inw(uint16_t port) {
	return port_word_in(port);
}

uint32_t laihost_ind(uint16_t port) {
	return port_dword_in(port);
}

void laihost_pci_writeb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun,
						uint16_t offset, uint8_t val) {
	pci_write(seg, bus, slot, fun, offset, val, 1);
}

void laihost_pci_writew(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun,
						uint16_t offset, uint16_t val) {
	pci_write(seg, bus, slot, fun, offset, val, 2);
}

void laihost_pci_writed(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun,
						uint16_t offset, uint32_t val) {
	pci_write(seg, bus, slot, fun, offset, val, 4);
}

uint8_t laihost_pci_readb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun,
						  uint16_t offset) {
	return pci_read(seg, bus, slot, fun, offset, 1);
}

uint16_t laihost_pci_readw(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun,
						   uint16_t offset) {
	return pci_read(seg, bus, slot, fun, offset, 2);
}

uint32_t laihost_pci_readd(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun,
						   uint16_t offset) {
	return pci_read(seg, bus, slot, fun, offset, 4);
}

void laihost_sleep(uint64_t) {}

void *laihost_scan(const char *signature, size_t index) {
	// The DSDT must be found using a pointer in the FADT
	if (!memcmp(signature, "DSDT", 4)) {
		struct facp *facp = (struct facp *)acpi_find_sdt("FACP", 0);

		return (void *)(uintptr_t)facp->Dsdt + MEM_PHYS_OFFSET;
	} else {
		return acpi_find_sdt(signature, index);
	}
}
