#ifndef PCI_H
#define PCI_H

/*
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

#include "../acpi/acpi.h"
#include "../klibc/dynarray.h"
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

DYNARRAY_EXTERN(struct mcfg_entry *, mcfg_entries);

void pci_init(void);
void pci_write(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function,
			   uint16_t offset, uint32_t value, uint8_t access_size);
uint32_t pci_read(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function,
				  uint16_t offset, uint8_t access_size);

#endif
