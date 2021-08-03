#include <stddef.h>
#include <stdint.h>
#include "../klibc/dynarray.h"
#include "../klibc/printf.h"
#include "../kernel/panic.h"
#include "fadt.h"
#include "acpi.h"
#include "../sys/mmio.h"
#include "../mm/vmm.h"

struct facp *facp;

void init_fadt(){
    // search for FADT table
    facp = acpi_find_sdt("FACP");
    if (!facp){
       PANIC("FACP tabel cannot be found.");
    }

    // search the \_S5 package in the DSDT
    uint8_t *S5Addr = (uint8_t *) facp->Dsdt + 36 + MEM_PHYS_OFFSET; // skip header
    
    printf("FACP tabel address: %X\n", (uint32_t)facp);
    int offset=facp->Dsdt + MEM_PHYS_OFFSET;
    printf("FACP dsdt address: %X\n", offset);
    struct sdt* header = (struct sdt*)(offset);
    int dsdtLength=header->length;
    printf("dsdt len: %X\n", dsdtLength);
    while (0 < dsdtLength--)
    {
      printf("a");
        if (S5Addr[0] == 0x44 && S5Addr[1] == 0x53 && S5Addr[2] == 0x44 && S5Addr[3] == 0x54)
            break;
            printf("b");
        S5Addr++;
             printf("c");
    }

    // check if \_S5 was found
    if (dsdtLength > 0)
    {

    }
    else{
         printf("acpi: _S5 object not found!\n");
    }
}