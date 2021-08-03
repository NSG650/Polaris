#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../klibc/printf.h"
#include "../mm/vmm.h"
#include "acpi.h"
#include "madt.h"
#include "../klibc/string.h"

struct rsdp {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t rev;
    uint32_t rsdt_addr;
    // ver 2.0 only
    uint32_t length;
    uint64_t xsdt_addr;
    uint8_t ext_checksum;
    uint8_t reserved[3];
} __attribute__((packed));

struct rsdt {
    struct sdt sdt;
    uint32_t* ptrs_start;
} __attribute__((packed));

static bool use_xsdt;
static struct rsdt *rsdt;

/* This function should look for all the ACPI tables and index them for
   later use */
void acpi_init(struct rsdp *rsdp) {
    printf("acpi: Revision: %d\n", rsdp->rev);

    if (rsdp->rev >= 2 && rsdp->xsdt_addr) {
        use_xsdt = true;
        rsdt = (struct rsdt *)((uintptr_t)rsdp->xsdt_addr + MEM_PHYS_OFFSET);
        printf("acpi: Found XSDT at %X\n", (uintptr_t)rsdt);
    } else {
        use_xsdt = false;
        rsdt = (struct rsdt *)((uintptr_t)rsdp->rsdt_addr + MEM_PHYS_OFFSET);
        printf("acpi: Found RSDT at %X\n", (uintptr_t)rsdt);
    }
    // Initialised individual tables that need initialisation
    init_madt();
    init_fadt();
}

/* Find SDT by signature */
void *acpi_find_sdt(const char *signature) {
    int len = rsdt->sdt.length;
    int entries = (len - 36) / 4;
    uint32_t reader = (uint32_t)rsdt->ptrs_start;
    for (size_t i = 0; i < entries; i++)
    {
         struct sdt *ptr = (struct sdt*)reader;
         if (!strncmp(ptr->signature, signature, 4)) {
            return (void*)ptr;
         }
         reader+=ptr->length;
    }

    printf("acpi: \"%s\" not found\n", signature);
    return NULL;
}
