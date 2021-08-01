#include <stddef.h>
#include <stdint.h>
#include "../klibc/dynarray.h"
#include "../klibc/debug.h"
#include "madt.h"
#include "acpi.h"

struct madt *madt;

DYNARRAY_GLOBAL(madt_local_apics);
DYNARRAY_GLOBAL(madt_io_apics);
DYNARRAY_GLOBAL(madt_isos);
DYNARRAY_GLOBAL(madt_nmis);

void init_madt(void) {
    // search for MADT table
    madt = acpi_find_sdt("APIC", 0);

    // parse the MADT entries
    for (uint8_t *madt_ptr = (uint8_t *)madt->madt_entries_begin;
        (uintptr_t)madt_ptr < (uintptr_t)madt + 1000;
        madt_ptr += *(madt_ptr + 1)) {
        switch (*(madt_ptr)) {
            case 0:
                // processor local APIC
                kprintf("acpi/madt: Found local APIC 0x%X\n", madt_local_apics.length);
                DYNARRAY_PUSHBACK(madt_local_apics, (void *)madt_ptr);
                break;
            case 1:
                // I/O APIC
                kprintf("acpi/madt: Found I/O APIC 0x%X\n", madt_io_apics.length);
                DYNARRAY_PUSHBACK(madt_io_apics, (void *)madt_ptr);
                break;
            case 2:
                // interrupt source override
                kprintf("acpi/madt: Found ISO 0x%X\n", madt_isos.length);
                DYNARRAY_PUSHBACK(madt_isos, (void *)madt_ptr);
                break;
            case 4:
                // NMI
                kprintf("acpi/madt: Found NMI 0x%X\n", madt_nmis.length);
                DYNARRAY_PUSHBACK(madt_nmis, (void *)madt_ptr);
                break;
        }
    }
}
