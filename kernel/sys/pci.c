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

#include "pci.h"
#include "../cpu/ports.h"
#include "../klibc/alloc.h"
#include "../klibc/printf.h"
#include "../mm/vmm.h"
#include "mmio.h"
#include <stdint.h>

struct pci_device *pci_devices[100];
int i = 0;
DYNARRAY_GLOBAL(mcfg_entries);

static uint32_t (*internal_read)(uint16_t, uint8_t, uint8_t, uint8_t, uint16_t,
								 uint8_t);
static void (*internal_write)(uint16_t, uint8_t, uint8_t, uint8_t, uint16_t,
							  uint32_t, uint8_t);

static uint32_t make_pci_address(uint32_t bus, uint32_t slot, uint32_t function,
								 uint16_t offset) {
	return ((bus << 16) | (slot << 11) | (function << 8) | (offset & 0xFFFC) |
			(1u << 31));
}

static uint32_t legacy_pci_read(uint16_t seg, uint8_t bus, uint8_t slot,
								uint8_t function, uint16_t offset,
								uint8_t access_size) {
	(void)seg;
	port_dword_out(0xCF8, make_pci_address(bus, slot, function, offset));
	switch (access_size) {
		case 1:
			return port_byte_in(0xCFC + (offset & 3));
		case 2:
			return port_word_in(0xCFC + (offset & 2));
		case 4:
			return port_dword_in(0xCFC);
		default:
			printf("PCI: Unknown access size: %hhu\n", access_size);
			return 0;
	}
}

static void legacy_pci_write(uint16_t seg, uint8_t bus, uint8_t slot,
							 uint8_t function, uint16_t offset, uint32_t value,
							 uint8_t access_size) {
	(void)seg;
	port_dword_out(0xCF8, make_pci_address(bus, slot, function, offset));
	switch (access_size) {
		case 1:
			port_byte_out(0xCFC + (offset & 3), value);
			break;
		case 2:
			port_word_out(0xCFC + (offset & 2), value);
			break;
		case 4:
			port_dword_out(0xCFC, value);
			break;
		default:
			printf("PCI: Unknown access size: %hhu\n", access_size);
			break;
	}
}

static uint32_t mcfg_pci_read(uint16_t seg, uint8_t bus, uint8_t slot,
							  uint8_t function, uint16_t offset,
							  uint8_t access_size) {
	for (size_t i = 0; i < mcfg_entries.length; i++) {
		struct mcfg_entry *entry = mcfg_entries.storage[i];
		if (entry->seg == seg) {
			if (bus >= entry->start_bus_number &&
				bus <= entry->end_bus_number) {
				void *addr =
					(void *)(((entry->base +
							   (((bus - entry->start_bus_number) << 20) |
								(slot << 15) | (function << 12))) |
							  offset) +
							 MEM_PHYS_OFFSET);
				switch (access_size) {
					case 1: {
						return mminb(addr);
					}

					case 2: {
						return mminw(addr);
					}

					case 4: {
						return mmind(addr);
					}

					default:
						printf("PCI: Unknown access size: %hhu\n", access_size);
						break;
				}
			}
		}
	}

	printf("PCI: Tried to read from nonexistent device, %hx:%hhx:%hhx:%hhx\n",
		   seg, bus, slot, function);
	return 0;
}

static void mcfg_pci_write(uint16_t seg, uint8_t bus, uint8_t slot,
						   uint8_t function, uint16_t offset, uint32_t value,
						   uint8_t access_size) {
	for (size_t i = 0; i < mcfg_entries.length; i++) {
		struct mcfg_entry *entry = mcfg_entries.storage[i];
		if (entry->seg == seg) {
			if (bus >= entry->start_bus_number &&
				bus <= entry->end_bus_number) {
				void *addr =
					(void *)(((entry->base +
							   (((bus - entry->start_bus_number) << 20) |
								(slot << 15) | (function << 12))) +
							  offset) +
							 MEM_PHYS_OFFSET);
				switch (access_size) {
					case 1: {
						mmoutb(addr, value);
						break;
					}

					case 2: {
						mmoutw(addr, value);
						break;
					}

					case 4: {
						mmoutd(addr, value);
						break;
					}

					default:
						printf("PCI: Unknown access size: %hhu\n", access_size);
						break;
				}
				return;
			}
		}
	}

	printf("PCI: Tried to write to nonexistent device, %hx:%hhx:%hhx:%hhx\n",
		   seg, bus, slot, function);
}

uint32_t pci_read(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function,
				  uint16_t offset, uint8_t access_size) {
	return internal_read(seg, bus, slot, function, offset, access_size);
}

void pci_write(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function,
			   uint16_t offset, uint32_t value, uint8_t access_size) {
	internal_write(seg, bus, slot, function, offset, value, access_size);
}

uint32_t pci_getvendor(uint8_t bus, uint8_t slot) {
	return pci_read(0, bus, slot, 0, 0, 2);
}
uint32_t pci_getdevice(uint8_t bus, uint8_t slot) {
	return pci_read(0, bus, slot, 0, 2, 2);
}
uint32_t pci_getheadertype(uint8_t bus, uint8_t slot) {
	return pci_read(0, bus, slot, 0, 0x0E, 2);
}

bool pci_checkIfDeviceExists(uint8_t bus, uint8_t device) {
	uint16_t vendorID = pci_getvendor(bus, device);
	if (vendorID == 0xFFFF)
		return false; // Device doesn't exist
	else
		return true;
}

struct pci_device *pci_getDevice(uint8_t bus, uint8_t device) {
	uint16_t vendorid = pci_getvendor(bus, device);
	if (vendorid == 0xFFFF)
		return NULL; // Device doesn't exist
	uint16_t deviceid = pci_getdevice(bus, device);
	uint8_t classCode = pci_read(0, bus, device, 0, 0x0b, 1);
	uint8_t subclass = pci_read(0, bus, device, 0, 0x0a, 1);
	uint8_t progintf = pci_read(0, bus, device, 0, 0x09, 1);

	uint16_t headerType = pci_getheadertype(bus, device);
	uint32_t functionCount = headerType & 0x80 ? 8 : 1;
	printf("Pci: found device: VendorID: %X DeviceID: %X Class code: %X Sub "
		   "class: %X progIntf: %X\n",
		   vendorid, deviceid, classCode, subclass, progintf);

	struct pci_device *pcidevice = alloc(sizeof(struct pci_device));
	pcidevice->vendorid = vendorid;
	pcidevice->deviceid = deviceid;
	pcidevice->classcode = classCode;
	pcidevice->subclass = subclass;
	pcidevice->progintf = progintf;
	pcidevice->bus = bus;
	pcidevice->device = device;
	pcidevice->functionCount = functionCount;
	return pcidevice;
}

void checkBus(uint8_t bus) {
	uint8_t device;

	for (device = 0; device < 32; device++) {
		struct pci_device *dev = pci_getDevice(bus, device);
		if (dev != NULL) {
			dev->id = i;
			pci_devices[i] = dev;
			i++;
		}
	}
}

void pci_init(void) {
	struct mcfg *mcfg = (struct mcfg *)acpi_find_sdt("MCFG", 0);
	if (mcfg == NULL) {
		internal_read = legacy_pci_read;
		internal_write = legacy_pci_write;
	} else if (mcfg->header.length < sizeof(acpi_header_t) + sizeof(uint64_t) +
										 sizeof(struct mcfg_entry)) {
		// There isn't any entry in the MCFG, assume legacy PCI
		internal_read = legacy_pci_read;
		internal_write = legacy_pci_write;
	} else {
		const size_t entries =
			(mcfg->header.length - (sizeof(acpi_header_t) + sizeof(uint64_t))) /
			sizeof(struct mcfg_entry);
		for (uint64_t i = 0; i < entries; i++) {
			DYNARRAY_PUSHBACK(mcfg_entries, (void *)&mcfg->entries[i]);
		}

		internal_read = mcfg_pci_read;
		internal_write = mcfg_pci_write;
	}

	// Scan PCI Devices
	uint8_t function;
	uint8_t bus;

	uint16_t headerType = pci_getheadertype(0, 0);
	if ((headerType & 0x80) == 0) {
		/* Single PCI host controller */
		checkBus(0);
	} else {
		/* Multiple PCI host controllers */
		for (function = 0; function < 8; function++) {
			uint32_t vendorID = pci_read(0, 0, 0, function, 0, 2);
			if (vendorID != 0xFFFF)
				break;
			bus = function;
			checkBus(bus);
		}
	}
}
void pci_read_bar(uint32_t id, uint32_t index, uint32_t *address,
				  uint32_t *mask) {
	struct pci_device *dev = pci_devices[id];
	uint32_t reg = 0x10 + index * sizeof(uint32_t);

	// Get address
	*address = pci_read(0, dev->bus, dev->device, 0, reg, 4);

	// Find the size of the bar
	pci_write(0, dev->bus, dev->device, 0, reg, 0xffffffff, 4);
	*mask = pci_read(0, dev->bus, dev->device, 0, reg, 4);

	// Restore adddress
	pci_write(0, dev->bus, dev->device, 0, reg, (uint32_t)address, 4);
}

void PciGetBar(struct pci_bar *bar, uint32_t id, uint32_t index) {
	// Read pci bar register
	uint32_t addressLow;
	uint32_t maskLow;
	pci_read_bar(id, index, &addressLow, &maskLow);

	if (addressLow & 0x04) {
		// 64-bit mmio
		uint32_t addressHigh;
		uint32_t maskHigh;
		pci_read_bar(id, index + 1, &addressHigh, &maskHigh);

		bar->u.address =
			(void *)(((uintptr_t)addressHigh << 32) | (addressLow & ~0xf));
		bar->size = ~(((uint64_t)maskHigh << 32) | (maskLow & ~0xf)) + 1;
		bar->flags = addressLow & 0xf;
	} else if (addressLow & 0x01) {
		// IO register
		bar->u.port = (uint16_t)(addressLow & ~0x3);
		bar->size = (uint16_t)(~(maskLow & ~0x3) + 1);
		bar->flags = addressLow & 0x3;
	} else {
		// 32-bit mmio
		bar->u.address = (void *)(uintptr_t)(addressLow & ~0xf);
		bar->size = ~(maskLow & ~0xf) + 1;
		bar->flags = addressLow & 0xf;
	}
}
