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

typedef struct pci_device
{
	uint32_t id;
    uint16_t vendorid;
    uint16_t deviceid;

    uint8_t classcode;
    uint8_t subclass;
    uint8_t progintf;

	//Device location
	uint8_t bus;
	uint8_t device;
	uint32_t functionCount;

} pci_device;
typedef struct pci_bar
{
    union
    {
        void *address;
        uint16_t port;
    } u;
    uint64_t size;
    uint32_t flags;
} pci_bar;

extern struct pci_device* pci_devices[100];
void PciGetBar(struct pci_bar *bar, uint32_t id, uint32_t index);
void pci_init(void);
void pci_write(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function,
			   uint16_t offset, uint32_t value, uint8_t access_size);
uint32_t pci_read(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function,
				  uint16_t offset, uint8_t access_size);

#endif
