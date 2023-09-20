#ifndef PCI_H
#define PCI_H

#include <stdint.h>

#if !(defined(__x86_64__))
#error "THIS IS ONLY FOR x86_64"
#endif

typedef struct acpi_header_t {
	char signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oem[6];
	char oem_table[8];
	uint32_t oem_revision;
	uint32_t creator_id;
	uint32_t creator_revision;
} __attribute__((packed)) acpi_header_t;

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

void pci_write(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function,
			   uint16_t offset, uint32_t value, uint8_t access_size);
uint32_t pci_read(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function,
				  uint16_t offset, uint8_t access_size);
struct pci_device *pci_get_pci_device(uint16_t vendor_id, uint16_t device_id);

#endif
