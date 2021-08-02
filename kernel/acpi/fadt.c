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
       panic("FACP tabel cannot be found.", "kernel/acpi/fadt.c", 0, 17);
    }

  // search the \_S5 package in the DSDT
               char *S5Addr = (char *) facp->Dsdt+MEM_PHYS_OFFSET +36; // skip header
               printf("dsdt address: %X\n", facp->Dsdt + MEM_PHYS_OFFSET);
               struct sdt* header = (struct sdt*)(facp->Dsdt + MEM_PHYS_OFFSET);
               //printf("%X", header->length);
              //// int dsdtLength=header->length;
              // while (0 < dsdtLength--)
               //{
               //   if ( strncmp(S5Addr, "_S5_", 4) == 0)
               //      break;
               //   S5Addr++;
               //}

    // check if \_S5 was found
    //if (dsdtLength > 0)
    //{

    //}
   // else{
    //     printf("acpi: _S5 object not found!");
    //}
}