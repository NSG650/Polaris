#ifndef ACPI_H
#define ACPI_H

#include <stdint.h>
#include <stddef.h>

#include "madt.h"

struct rsdp;

struct sdt {
    char signature[4];
    uint32_t length;
    uint8_t rev;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_rev;
    uint32_t creator_id;
    uint32_t creator_rev;
} __attribute__((packed));


void acpi_init(struct rsdp *rsdp);
void *acpi_find_sdt(const char *signature);

#endif
