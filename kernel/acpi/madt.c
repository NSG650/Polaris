#include <stddef.h>
#include <stdint.h>
#include "../klibc/dynarray.h"
#include "../klibc/printf.h"
#include "../kernel/panic.h"
#include "madt.h"
#include "acpi.h"

struct madt *madt;

DYNARRAY_GLOBAL(madt_local_apics);
DYNARRAY_GLOBAL(madt_io_apics);
DYNARRAY_GLOBAL(madt_isos);
DYNARRAY_GLOBAL(madt_nmis);

void init_madt(void) {
    // search for MADT table
    madt = acpi_find_sdt("APIC");
    if (!madt){
         PANIC("MADT tabel cannot be found.");
         return;
    }
    //printf("%p\n", (uint8_t *)madt->madt_entries_begin);
    // parse the MADT entries
    for (uint8_t *madt_ptr = (uint8_t *)madt->madt_entries_begin;
        (uintptr_t)madt_ptr < (uintptr_t)madt + sizeof(madt);
        madt_ptr += *(madt_ptr + 1)) {
        switch (*(madt_ptr)) {
            case 0:
                // processor local APIC
                printf("acpi/madt: Found local APIC 0x%X\n", madt_local_apics.length);
                DYNARRAY_PUSHBACK(madt_local_apics, (void *)madt_ptr);
                break;
            case 1:
                // I/O APIC
                printf("acpi/madt: Found I/O APIC 0x%X\n", madt_io_apics.length);
                DYNARRAY_PUSHBACK(madt_io_apics, (void *)madt_ptr);
                break;
            case 2:
                // interrupt source override
                printf("acpi/madt: Found ISO 0x%X\n", madt_isos.length);
                DYNARRAY_PUSHBACK(madt_isos, (void *)madt_ptr);
                break;
            case 4:
                // NMI
                printf("acpi/madt: Found NMI 0x%X\n", madt_nmis.length);
                DYNARRAY_PUSHBACK(madt_nmis, (void *)madt_ptr);
                break;
        }
    }
}
