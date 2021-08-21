#ifndef ACPI_H
#define ACPI_H

#include <stddef.h>
#include <stdint.h>

struct sdt {
	char signature[4];
	uint32_t length;
	uint8_t rev;
	uint8_t checksum;
	char oem_id[6];
	char oem_table_id[8];
	uint32_t oem_rev;
	char creator_id[4];
	uint32_t creator_rev;
} __attribute__((packed));

struct rsdp {
	char signature[8];
	uint8_t checksum;
	char oem_id[6];
	uint8_t rev;
	uint32_t rsdt_addr;
	// Rev 2 only after this comment
	uint32_t length;
	uint64_t xsdt_addr;
	uint8_t ext_checksum;
	uint8_t reserved[3];
} __attribute__((packed));

struct rsdt {
	struct sdt header;
	char ptrs_start[];
} __attribute__((packed));

void acpi_init(struct rsdp *rsdp);
void *acpi_find_sdt(const char *signature, int index);
void acpi_start(void);
void acpi_shutdown(void);
void acpi_reboot(void);

#endif
