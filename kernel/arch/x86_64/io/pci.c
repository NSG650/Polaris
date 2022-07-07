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

#include <debug/debug.h>
#include <io/mmio.h>
#include <io/pci.h>
#include <io/ports.h>
#include <mem/liballoc.h>
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
	outportdw(0xCF8, make_pci_address(bus, slot, function, offset));
	switch (access_size) {
		case 1:
			return inportb(0xCFC + (offset & 3));
		case 2:
			return inportw(0xCFC + (offset & 2));
		case 4:
			return inportdw(0xCFC);
		default:
			kprintf("PCI: Unknown access size: 0x%x\n", access_size);
			return 0;
	}
}

static void legacy_pci_write(uint16_t seg, uint8_t bus, uint8_t slot,
							 uint8_t function, uint16_t offset, uint32_t value,
							 uint8_t access_size) {
	(void)seg;
	outportdw(0xCF8, make_pci_address(bus, slot, function, offset));
	switch (access_size) {
		case 1:
			outportb(0xCFC + (offset & 3), value);
			break;
		case 2:
			outportw(0xCFC + (offset & 2), value);
			break;
		case 4:
			outportdw(0xCFC, value);
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
						return inmmb(addr);
					}

					case 2: {
						return inmmw(addr);
					}

					case 4: {
						return inmmdw(addr);
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
						outmmb(addr, value);
						break;
					}

					case 2: {
						outmmw(addr, value);
						break;
					}

					case 4: {
						outmmdw(addr, value);
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

uint32_t pci_read(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function,
				  uint16_t offset, uint8_t access_size) {
	return internal_read(seg, bus, slot, function, offset, access_size);
}

void pci_write(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function,
			   uint16_t offset, uint32_t value, uint8_t access_size) {
	internal_write(seg, bus, slot, function, offset, value, access_size);
}
