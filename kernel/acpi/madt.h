#ifndef MADT_H
#define MADT_H
#include <stdint.h>
#include <stddef.h>
#include "../klibc/mem.h"
#include "acpi.h"
#include "../klibc/dynarray.h"

struct madt {
    struct sdt sdt;
    uint32_t local_controller_addr;
    uint32_t flags;
    symbol madt_entries_begin;
} __attribute__((packed));

struct madt_header {
    uint8_t type;
    uint8_t length;
} __attribute__((packed));

struct madt_lapic {
    struct madt_header madtHeader;
    uint8_t    processor_id;
    uint8_t    apic_id;
    uint32_t   flags;
} __attribute__((packed));

struct madt_ioapic {
    struct madt_header madtHeader;
    uint8_t    apic_id;
    uint8_t    reserved;
    uint32_t   addr;
    uint32_t   gsib;
} __attribute__((packed));

struct madt_iso {
    struct madt_header madtHeader;
    uint8_t    bus_source;
    uint8_t    irq_source;
    uint32_t   gsi;
    uint16_t   flags;
} __attribute__((packed));

struct madt_nmi {
    struct madt_header madtHeader;
    uint8_t    processor;
    uint16_t   flags;
    uint8_t    lint;
} __attribute__((packed));

DYNARRAY_EXTERN(struct madt_lapic *,  madt_local_apics);
DYNARRAY_EXTERN(struct madt_ioapic *, madt_io_apics);
DYNARRAY_EXTERN(struct madt_iso *,    madt_isos);
DYNARRAY_EXTERN(struct madt_nmi *,    madt_nmis);

void init_madt(void);

#endif
