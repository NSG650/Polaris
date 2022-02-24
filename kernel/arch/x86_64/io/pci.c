#include <debug/debug.h>
#include <io/mmio.h>
#include <io/pci.h>
#include <io/ports.h>
#include <mm/vmm.h>

struct mcfg_entry *mcfg_entries;

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
	for (size_t i = 0; i < vector_size(mcfg_entries); i++) {
		struct mcfg_entry *entry = &mcfg_entries[i];
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
	for (size_t i = 0; i < vector_size(mcfg_entries); i++) {
		struct mcfg_entry *entry = &mcfg_entries[i];
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

void pci_init(void) {
	struct mcfg *mcfg = (struct mcfg *)acpi_find_sdt("MCFG", 0);
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
		mcfg_entries = vector_create();
		const size_t entries = (mcfg->header.length - sizeof(struct mcfg)) /
							   sizeof(struct mcfg_entry);
		for (size_t i = 0; i < entries; i++) {
			vector_add(&mcfg_entries, mcfg->entries[i]);
		}
		kprintf("PCI: Using MCFG Read/Write\n");
		internal_read = mcfg_pci_read;
		internal_write = mcfg_pci_write;
	}
}

uint32_t pci_read(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function,
				  uint16_t offset, uint8_t access_size) {
	return internal_read(seg, bus, slot, function, offset, access_size);
}

void pci_write(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function,
			   uint16_t offset, uint32_t value, uint8_t access_size) {
	internal_write(seg, bus, slot, function, offset, value, access_size);
}
