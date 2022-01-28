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

#include "../cpu/ports.h"
#include "../kernel/panic.h"
#include "../klibc/printf.h"
#include "../mm/vmm.h"
#include "../sys/hpet.h"
#include "../sys/pci.h"
#include "madt.h"
#include <lai/core.h>
#include <lai/drivers/ec.h>
#include <lai/helpers/sci.h>
#include <lai/host.h>
#include <liballoc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>

static bool use_xsdt;
static struct rsdt *rsdt;
static uint8_t revision;

static void init_ec(void) {
	LAI_CLEANUP_STATE lai_state_t state;
	lai_init_state(&state);

	LAI_CLEANUP_VAR lai_variable_t pnp_id = LAI_VAR_INITIALIZER;
	lai_eisaid(&pnp_id, ACPI_EC_PNP_ID);

	struct lai_ns_iterator it = LAI_NS_ITERATOR_INITIALIZER;
	lai_nsnode_t *node;
	while ((node = lai_ns_iterate(&it))) {
		if (lai_check_device_pnp_id(node, &pnp_id, &state)) // This is not an EC
			continue;

		// Found one
		struct lai_ec_driver *driver = kmalloc(sizeof(
			struct lai_ec_driver)); // Dynamically allocate the memory since -
		lai_init_ec(node, driver);	// we dont know how many ECs there could be

		struct lai_ns_child_iterator child_it =
			LAI_NS_CHILD_ITERATOR_INITIALIZER(node);
		lai_nsnode_t *child_node;
		while ((child_node = lai_ns_child_iterate(&child_it))) {
			if (lai_ns_get_node_type(child_node) == LAI_NODETYPE_OPREGION)
				lai_ns_override_opregion(child_node, &lai_ec_opregion_override,
										 driver);
		}
	}
}

void acpi_init(acpi_xsdp_t *rsdp) {
	printf("ACPI: Revision: %hhu\n", rsdp->revision);
	revision = rsdp->revision;

	if (rsdp->revision >= 2 && rsdp->xsdt) {
		use_xsdt = true;
		rsdt = (struct rsdt *)((uintptr_t)rsdp->xsdt + MEM_PHYS_OFFSET);
		printf("ACPI: Found XSDT at %llX\n", (uintptr_t)rsdt);
	} else {
		use_xsdt = false;
		rsdt = (struct rsdt *)((uintptr_t)rsdp->rsdt + MEM_PHYS_OFFSET);
		printf("ACPI: Found RSDT at %llX\n", (uintptr_t)rsdt);
	}
	hpet_init();
	pci_init();
	lai_set_acpi_revision(revision);
	lai_create_namespace();
	lai_enable_acpi(1);
	init_madt();
	init_ec();
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

noreturn void laihost_panic(const char *msg) {
	PANIC(msg);
	__builtin_unreachable();
}

void *laihost_malloc(size_t size) {
	return kmalloc(size);
}

void *laihost_realloc(void *ptr, size_t newsize, size_t oldsize) {
	(void)oldsize;
	return krealloc(ptr, newsize);
}

void laihost_free(void *ptr, size_t oldsize) {
	(void)oldsize;
	kfree(ptr);
}

void *laihost_map(size_t address, size_t size) {
	(void)size;
	return (void *)address + MEM_PHYS_OFFSET;
}

void laihost_unmap(void *pointer, size_t count) {
	// We don't want to unmap kernel mapped memory
	(void)pointer;
	(void)count;
}

__attribute__((always_inline)) inline bool is_canonical(uint64_t addr) {
	return ((addr <= 0x00007FFFFFFFFFFF) ||
			((addr >= 0xFFFF800000000000) && (addr <= 0xFFFFFFFFFFFFFFFF)));
}

void *laihost_scan(const char *signature, size_t index) {
	// The DSDT must be found using a pointer in the FADT
	if (!memcmp(signature, "DSDT", 4)) {
		acpi_fadt_t *facp = acpi_find_sdt("FACP", 0);
		uint64_t dsdt_addr = 0;

		if (is_canonical(facp->x_dsdt) && revision >= 2) {
			dsdt_addr = facp->x_dsdt;
		} else {
			dsdt_addr = facp->dsdt;
		}

		return (void *)dsdt_addr + MEM_PHYS_OFFSET;
	} else {
		return acpi_find_sdt(signature, index);
	}
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

void laihost_sleep(uint64_t ms) {
	// Convert to microseconds
	hpet_usleep(ms * 1000);
}

uint64_t laihost_timer(void) {
	// Convert to 100 nanosecond units
	return hpet_counter_value() / 100000000;
}
