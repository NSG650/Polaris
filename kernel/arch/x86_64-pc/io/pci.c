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
#include <sys/prcb.h>

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

	// Checking for capabilities;
	uint16_t sreg = PCI_READ_W(device_struct, 6);

	if (sreg & (1 << 4)) {
		uint8_t next_offset = PCI_READ_B(device_struct, 0x34);

		while (next_offset) {
			uint8_t identifier = PCI_READ_B(device_struct, next_offset);

			switch (identifier) {
				case 5: // MSI
					device_struct->msi_supported = true;
					device_struct->msi_offset = next_offset;
					break;
				case 16: // PCIe compatible
					device_struct->pcie_supported = true;
					device_struct->pcie_supported = next_offset;
					break;
				case 17: // MSI-X
					device_struct->msix_supported = true;
					device_struct->msix_offset = next_offset;
					break;
			}

			next_offset = PCI_READ_B(device_struct, next_offset + 1);
		}
	}

	kprintf("PCI: Got Device at %x:%x vendor id: %x classcode: "
			"\"%s\" subclass: %x "
			"progintf: %x device_id: %x msi_supported: %c msix_supported: %c "
			"pcie_supported: %c\n",
			bus, device, device_struct->vendor_id,
			pci_get_classcode_name(device_struct->classcode),
			device_struct->subclass, device_struct->progintf,
			device_struct->device_id, device_struct->msi_supported ? 'T' : 'F',
			device_struct->msix_supported ? 'T' : 'F',
			device_struct->pcie_supported ? 'T' : 'F');

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
			vmm_unmap_page(kernel_pagemap, start + offset,
						   false); // without this vmm_map_page will fail
								   // if a part of the bar was mapped
			vmm_unmap_page(kernel_pagemap, start + offset + MEM_PHYS_OFFSET,
						   false);
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

	uint16_t offset = PCI_OFFSET_BAR0 + n * sizeof(uint32_t);

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

bool pci_setup_irq(struct pci_device *dev, size_t index, uint8_t vector) {
	cli();
	union msi_address addr = {.dest_id = prcb_return_current_cpu()->lapic_id,
							  .base_address = 0xfee};
	union msi_data data = {.vector = vector, .delivery = 0};
	sti();

	if (dev->msix_supported) {
		uint16_t control = PCI_READ_W(dev, dev->msix_offset + 2);
		control |= (0b11 << 14);
		PCI_WRITE_W(dev, dev->msix_offset + 2, control);

		uint16_t n_irqs = (control & ((1 << 11) - 1)) + 1;
		uint32_t info = PCI_READ_D(dev, dev->msix_offset + 4);
		if (index > n_irqs) {
			return false;
		}

		struct pci_bar bar = {0};

		if (!pci_get_bar_n(dev, &bar, info & 0b111)) {
			return false;
		}

		if (!bar.is_mmio || !bar.base) {
			return false;
		}

		uintptr_t target = bar.base + (info & ~0b111) + MEM_PHYS_OFFSET;
		target += index * 16;
		((volatile uint64_t *)target)[0] = addr.raw;
		((volatile uint32_t *)target)[2] = data.raw;

		// Clear both global/local masks, and put MSI-X back in operation
		((volatile uint32_t *)target)[3] = 0;
		PCI_WRITE_W(dev, dev->msix_offset + 2, control & ~(1 << 14));

		return true;
	}

	if (dev->msi_supported) {
		uint16_t control = PCI_READ_W(dev, dev->msi_offset + 2) | 1;
		uint8_t data_off = (control & (1 << 7)) ? 0xc : 0x8;
		if ((control >> 1) & 0b111) {
			control &= ~(0b111 << 4); // Set MME to 0 (enable only 1 IRQ)
		}

		PCI_WRITE_D(dev, dev->msi_offset + 4, addr.raw);
		PCI_WRITE_W(dev, dev->msi_offset + data_off, data.raw);
		PCI_WRITE_W(dev, dev->msi_offset + 2, control);

		return true;
	}

	return false;
}

bool pci_mask(struct pci_device *dev, size_t index, bool mask) {
	if (dev->msix_supported) {
		uint16_t control = PCI_READ_W(dev, dev->msix_offset + 2);
		control |= (0b11 << 14);
		PCI_WRITE_W(dev, dev->msix_offset + 2, control);

		uint16_t n_irqs = (control & ((1 << 11) - 1)) + 1;
		uint32_t info = PCI_READ_D(dev, dev->msix_offset + 4);
		if (index > n_irqs) {
			return false;
		}

		struct pci_bar bar = {0};

		if (!pci_get_bar_n(dev, &bar, info & 0b111)) {
			return false;
		}

		if (!bar.is_mmio || !bar.base) {
			return false;
		}

		uintptr_t target = bar.base + (info & ~0b111) + MEM_PHYS_OFFSET;
		target += index * 16;

		((volatile uint32_t *)target)[3] = (int)mask;

		return true;
	}

	if (dev->msi_supported) {
		uint16_t control = PCI_READ_W(dev, dev->msi_offset + 2) | 1;

		if (mask) {
			PCI_WRITE_W(dev, dev->msi_offset + 2, control & ~1);
		} else {
			PCI_WRITE_W(dev, dev->msi_offset + 2, control | 1);
		}

		return true;
	}

	return false;
}

uint32_t pci_read(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function,
				  uint16_t offset, uint8_t access_size) {
	return internal_read(seg, bus, slot, function, offset, access_size);
}

void pci_write(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function,
			   uint16_t offset, uint32_t value, uint8_t access_size) {
	internal_write(seg, bus, slot, function, offset, value, access_size);
}
