/*
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

#ifndef PCI_H
#define PCI_H

#include <fw/acpi.h>
#include <klibc/vec.h>
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
};

typedef vec_t(struct mcfg_entry) mcfg_vec_t;

extern mcfg_vec_t mcfg_entries;
extern struct pci_device *pci_devices[];

void pci_init(void);
void pci_write(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function,
			   uint16_t offset, uint32_t value, uint8_t access_size);
uint32_t pci_read(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function,
				  uint16_t offset, uint8_t access_size);
struct pci_device *pci_get_pci_device(uint16_t vendor_id, uint16_t device_id);

#endif
