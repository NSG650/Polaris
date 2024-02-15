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

#include <debug/debug.h>
#include <io/mmio.h>
#include <io/pci.h>
#include <io/ports.h>
#include <klibc/misc.h>
#include <mm/slab.h>
#include <mm/vmm.h>
#include <stdbool.h>

mcfg_vec_t mcfg_entries;
struct pci_device *pci_devices[256] = {0};

int dev_count = 0;

static uint32_t (*internal_read)(uint16_t, uint8_t, uint8_t, uint8_t, uint16_t,
								 uint8_t);
static void (*internal_write)(uint16_t, uint8_t, uint8_t, uint8_t, uint16_t,
							  uint32_t, uint8_t);

static uint32_t make_pci_address(uint32_t bus, uint32_t slot, uint32_t function,
								 uint16_t offset) {
	return ((bus << 16) | (slot << 11) | (function << 8) | (offset & 0xFFFC) |
			(1u << 31));
}

char *pci_class_code_name[] = {"Unclassified",
							   "Mass Storage Controller",
							   "Network Controller",
							   "Display Controller",
							   "Multimedia Controller",
							   "Memory Controller",
							   "Bridge",
							   "Simple Communication Controller",
							   "Base System Peripheral",
							   "Input Device Controller",
							   "Docking Station",
							   "Processor",
							   "Serial Bus Controller",
							   "Wireless Controller",
							   "Intelligent Controller",
							   "Satellite Communication Controller",
							   "Encryption Controller",
							   "Signal Processing Controller",
							   "Processing Accelerator",
							   "Non-Essential Instrumentation"};

static uint32_t legacy_pci_read(uint16_t seg, uint8_t bus, uint8_t slot,
								uint8_t function, uint16_t offset,
								uint8_t access_size) {
	(void)seg;
	outd(0xCF8, make_pci_address(bus, slot, function, offset));
	switch (access_size) {
		case 1:
			return inb(0xCFC + (offset & 3));
		case 2:
			return inw(0xCFC + (offset & 2));
		case 4:
			return ind(0xCFC);
		default:
			kprintf("PCI: Unknown access size: 0x%x\n", access_size);
			return 0;
	}
}

static void legacy_pci_write(uint16_t seg, uint8_t bus, uint8_t slot,
							 uint8_t function, uint16_t offset, uint32_t value,
							 uint8_t access_size) {
	(void)seg;
	outd(0xCF8, make_pci_address(bus, slot, function, offset));
	switch (access_size) {
		case 1:
			outb(0xCFC + (offset & 3), value);
			break;
		case 2:
			outw(0xCFC + (offset & 2), value);
			break;
		case 4:
			outd(0xCFC, value);
			break;
		default:
			kprintf("PCI: Unknown access size: 0x%x\n", access_size);
			break;
	}
}

static uint32_t mcfg_pci_read(uint16_t seg, uint8_t bus, uint8_t slot,
							  uint8_t function, uint16_t offset,
							  uint8_t access_size) {
	for (int i = 0; i < mcfg_entries.length; i++) {
		struct mcfg_entry *entry = &mcfg_entries.data[i];
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
						kprintf("PCI: Unknown access size: %x\n", access_size);
						break;
				}
			}
		}
	}

	kprintf("PCI: Tried to read from nonexistent device, %x:%x:%x:%x\n", seg,
			bus, slot, function);
	return 0;
}

static void mcfg_pci_write(uint16_t seg, uint8_t bus, uint8_t slot,
						   uint8_t function, uint16_t offset, uint32_t value,
						   uint8_t access_size) {
	for (int i = 0; i < mcfg_entries.length; i++) {
		struct mcfg_entry *entry = &mcfg_entries.data[i];
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
						kprintf("PCI: Unknown access size: %x\n", access_size);
						break;
				}
				return;
			}
		}
	}

	kprintf("PCI: Tried to write to nonexistent device, %x:%x:%x:%x\n", seg,
			bus, slot, function);
}

static uint32_t pci_get_vendor(uint8_t bus, uint8_t slot) {
	return pci_read(0, bus, slot, 0, 0, 2);
}

static uint32_t pci_get_device(uint8_t bus, uint8_t slot) {
	return pci_read(0, bus, slot, 0, 2, 2);
}

static uint32_t pci_get_header_type(uint8_t bus, uint8_t slot) {
	return pci_read(0, bus, slot, 0, 0xe, 2);
}

static bool pci_does_device_exist(uint8_t bus, uint8_t device) {
	uint16_t vendor_id = pci_get_vendor(bus, device);
	return !(vendor_id == 0xffff);
}

static char *pci_get_classcode_name(uint8_t classcode) {
	if (classcode < 0x14) {
		return pci_class_code_name[classcode];
	} else if ((classcode > 0x13 && classcode < 0x40) ||
			   (classcode > 0x40 && classcode < 0xff)) {
		return "Reserved";
	} else if (classcode == 0x40) {
		return "Co-Processor";
	} else {
		return "Unassigned";
	}
}

struct pci_device *pci_get_device_info(uint8_t bus, uint8_t device) {
	if (!pci_does_device_exist(bus, device))
		return NULL;
	struct pci_device *device_struct = kmalloc(sizeof(struct pci_device));
	device_struct->bus = bus;
	device_struct->device = device;
	device_struct->vendor_id = pci_get_vendor(bus, device);
	device_struct->classcode = pci_read(0, bus, device, 0, 0xb, 1);
	device_struct->subclass = pci_read(0, bus, device, 0, 0xa, 1);
	device_struct->progintf = pci_read(0, bus, device, 0, 0x9, 1);
	device_struct->device_id = pci_get_device(bus, device);
	kprintf("PCI: Got Device at %x:%x vendor id: %x classcode: "
			"\"%s\" subclass: %x "
			"progintf: %x device_id: %x\n",
			bus, device, device_struct->vendor_id,
			pci_get_classcode_name(device_struct->classcode),
			device_struct->subclass, device_struct->progintf,
			device_struct->device_id);
	return device_struct;
}

struct pci_device *pci_get_pci_device(uint16_t vendor_id, uint16_t device_id) {
	for (int i = 0; i < 256; i++) {
		struct pci_device *dev = pci_devices[i];
		if (dev != NULL) {
			if (dev->device_id == device_id && dev->vendor_id == vendor_id)
				return dev;
		}
	}
	return NULL;
}

static void pci_check_bus(uint8_t bus) {
	uint8_t device;
	for (device = 0; device < 32; device++) {
		struct pci_device *dev = pci_get_device_info(bus, device);
		if (dev != NULL) {
			dev->id = dev_count;
			pci_devices[dev_count] = dev;
			dev_count++;
		}
	}
}

void pci_init(void) {
	struct mcfg *mcfg = (struct mcfg *)acpi_find_sdt("MCFG", 0);
	vec_init(&mcfg_entries);
	if (mcfg == NULL) {
		kprintf("PCI: Using Legacy Read/Write\n");
		internal_read = legacy_pci_read;
		internal_write = legacy_pci_write;
	} else if (mcfg->header.length <
			   sizeof(struct mcfg) + sizeof(struct mcfg_entry)) {
		kprintf("PCI: Using Legacy Read/Write\n");
		// There isn't any entry in the MCFG, assume legacy PCI
		internal_read = legacy_pci_read;
		internal_write = legacy_pci_write;
	} else {
		vec_init(&mcfg_entries);
		const size_t entries = (mcfg->header.length - sizeof(struct mcfg)) /
							   sizeof(struct mcfg_entry);
		for (size_t i = 0; i < entries; i++) {
			vec_push(&mcfg_entries, mcfg->entries[i]);
		}
		kprintf("PCI: Using MCFG Read/Write\n");
		internal_read = mcfg_pci_read;
		internal_write = mcfg_pci_write;
	}
	uint8_t function;
	uint8_t bus;
	uint16_t header_type = pci_get_header_type(0, 0);
	if (!(header_type & 0x80)) {
		kprintf("PCI: Single PCI Controller found\n");
		pci_check_bus(0);
	} else {
		kprintf("PCI: Multiple PCI Controller found\n");
		for (function = 0; function < 8; function++) {
			uint32_t vendor_id = pci_read(0, 0, 0, function, 0, 2);
			if (vendor_id == 0xffff)
				break;
			bus = function;
			pci_check_bus(bus);
		}
	}
	kprintf("PCI: Total number of devices on PCI: %d\n", dev_count);
}

static bool pci_map_bar(struct pci_bar *bar) {
	uintptr_t start = ALIGN_DOWN(bar->base, PAGE_SIZE);
	uintptr_t end = ALIGN_UP(bar->base + bar->size, PAGE_SIZE);
	bool mapped = true;

	for (uintptr_t offset = 0; offset < end - start && mapped;
		 offset += PAGE_SIZE) {
		if (vmm_virt_to_phys(kernel_pagemap, start + offset) == INVALID_PHYS) {
			mapped = false;
		}
	}

	if (mapped == false) {
		for (uintptr_t offset = 0; offset < end - start; offset += PAGE_SIZE) {
			vmm_unmap_page(kernel_pagemap,
						   start +
							   offset); // without this vmm_map_page will fail
										// if a part of the bar was mapped
			vmm_unmap_page(kernel_pagemap, start + offset + MEM_PHYS_OFFSET);
			if (vmm_map_page(kernel_pagemap, start + offset, start + offset,
							 PAGE_READ | PAGE_WRITE, Size4KiB) == false ||
				vmm_map_page(kernel_pagemap, start + offset + MEM_PHYS_OFFSET,
							 start + offset, PAGE_READ | PAGE_WRITE,
							 Size4KiB) == false) {
				return false;
			}
		}
	}

	return true;
}

bool pci_get_bar_n(struct pci_device *device, struct pci_bar *bar, uint8_t n) {
	if (n > 5)
		return false;

	uint16_t offset = 0x10 + n * sizeof(uint32_t);

	uint32_t base_low = pci_read(0, device->bus, device->device, 0, offset, 4);
	pci_write(0, device->bus, device->device, 0, offset, (uint32_t)(~0), 4);
	uint32_t size_low = pci_read(0, device->bus, device->device, 0, offset, 4);
	pci_write(0, device->bus, device->device, 0, offset, base_low, 4);

	if (base_low & 1) {
		bar->base = base_low & ~0b11;
		bar->size = (~size_low & ~0b11) + 1;
		bar->is_mmio = false;
	}

	else {
		int type = (base_low >> 1) & 3;
		uint32_t base_high =
			pci_read(0, device->bus, device->device, 0, offset + 4, 4);

		bar->base = base_low & 0xfffffff0;

		if (type == 2) {
			bar->base |= ((uint64_t)base_high << 32);
		}

		bar->size = ~(size_low & ~0b1111) + 1;
		bar->is_mmio = true;

		if (!pci_map_bar(bar)) {
			return false;
		}
	}

	return true;
}

uint32_t pci_read(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function,
				  uint16_t offset, uint8_t access_size) {
	return internal_read(seg, bus, slot, function, offset, access_size);
}

void pci_write(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function,
			   uint16_t offset, uint32_t value, uint8_t access_size) {
	internal_write(seg, bus, slot, function, offset, value, access_size);
}
