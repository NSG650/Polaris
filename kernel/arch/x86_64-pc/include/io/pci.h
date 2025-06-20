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

#ifndef PCI_H
#define PCI_H

#include <fw/acpi.h>
#include <klibc/vec.h>
#include <stdbool.h>
#include <stdint.h>

struct mcfg_entry {
	uint64_t base;
	uint16_t seg;
	uint8_t start_bus_number;
	uint8_t end_bus_number;
	uint32_t reserved;
} __attribute__((packed));

struct mcfg {
	acpi_header_t header;
	uint64_t reserved;
	struct mcfg_entry entries[];
} __attribute__((packed));

struct pci_device {
	uint32_t id;
	uint16_t vendor_id;
	uint16_t device_id;

	uint8_t classcode;
	uint8_t subclass;
	uint8_t progintf;

	uint8_t bus;
	uint8_t device;

	bool msi_supported;
	bool msix_supported;

	uint16_t msi_offset;
	uint16_t msix_offset;

	bool pcie_supported;
	uint16_t pcie_offset;
};

struct pci_bar {
	uintptr_t base;
	size_t size;
	bool is_mmio;
};

union msi_address {
	struct {
		uint32_t reserved0 : 2;
		uint32_t dest_mode : 1;
		uint32_t redir_hint : 1;
		uint32_t reserved1 : 8;
		uint32_t dest_id : 8;
		uint32_t base_address : 12;
	};

	uint32_t raw;
};

union msi_data {
	struct {
		uint32_t vector : 8;
		uint32_t delivery : 3;
		uint32_t reserved0 : 3;
		uint32_t level : 1;
		uint32_t trigger_mode : 1;
		uint32_t reserved1 : 16;
	};

	uint32_t raw;
};

#define PCI_OFFSET_COMMAND 0x04
#define PCI_OFFSET_STATUS 0x06
#define PCI_OFFSET_PROG_IF 0x09
#define PCI_OFFSET_HEADER_TYPE 0x0E
#define PCI_OFFSET_BASE_CLASS 0x0B
#define PCI_OFFSET_SUB_CLASS 0x0A
#define PCI_OFFSET_SECONDARY_BUS 0x19
#define PCI_OFFSET_BAR0 0x10
#define PCI_OFFSET_BAR1 0x14
#define PCI_OFFSET_BAR2 0x18
#define PCI_OFFSET_BAR3 0x1C
#define PCI_OFFSET_BAR4 0x20
#define PCI_OFFSET_BAR5 0x24
#define PCI_OFFSET_CAP_PTR 0x34
#define PCI_OFFSET_INT_LINE 0x3C
#define PCI_OFFSET_INT_PIN 0x3D

#define PCI_INT_PIN_NONE 0x00
#define PCI_INT_PIN_INTA 0x01
#define PCI_INT_PIN_INTB 0x02
#define PCI_INT_PIN_INTC 0x03
#define PCI_INT_PIN_INTD 0x04

typedef vec_t(struct mcfg_entry) mcfg_vec_t;

extern mcfg_vec_t mcfg_entries;
extern struct pci_device *pci_devices[];

void pci_init(void);
void pci_write(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function,
			   uint16_t offset, uint32_t value, uint8_t access_size);
uint32_t pci_read(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function,
				  uint16_t offset, uint8_t access_size);
struct pci_device *pci_get_pci_device(uint16_t vendor_id, uint16_t device_id);
bool pci_get_bar_n(struct pci_device *device, struct pci_bar *bar, uint8_t n);
bool pci_setup_irq(struct pci_device *dev, size_t index, uint8_t vector);
bool pci_mask(struct pci_device *dev, size_t index, bool mask);

#define PCI_READ_B(DEV, OFF) \
	(uint8_t)pci_read(0, DEV->bus, DEV->device, 0, OFF, 1)
#define PCI_READ_W(DEV, OFF) \
	(uint16_t)pci_read(0, DEV->bus, DEV->device, 0, OFF, 2)
#define PCI_READ_D(DEV, OFF) \
	(uint32_t)pci_read(0, DEV->bus, DEV->device, 0, OFF, 4)

#define PCI_WRITE_B(DEV, OFF, VAL) \
	pci_write(0, DEV->bus, DEV->device, 0, OFF, (uint8_t)VAL, 1)
#define PCI_WRITE_W(DEV, OFF, VAL) \
	pci_write(0, DEV->bus, DEV->device, 0, OFF, (uint16_t)VAL, 2)
#define PCI_WRITE_D(DEV, OFF, VAL) \
	pci_write(0, DEV->bus, DEV->device, 0, OFF, (uint32_t)VAL, 4)

#endif
