#include "fadt.h"
#include "../cpu/ports.h"
#include "../kernel/panic.h"
#include "../klibc/printf.h"
#include "../mm/vmm.h"
#include <stddef.h>

struct facp *facp;

uint8_t ACPI_ENABLE;
uint8_t ACPI_DISABLE;
uint8_t PM1_CNT_LEN;

uint16_t SLP_TYPa;
uint16_t SLP_TYPb;
uint16_t SLP_EN;
uint16_t SCI_EN;

uint32_t *SMI_CMD;
uint32_t PM1a_CNT;
uint32_t PM1b_CNT;

void init_fadt(void) {
	// Search for FADT table
	facp = acpi_find_sdt("FACP", 0);
	if (!facp) {
		PANIC("FADT table cannot be found.");
	}

	// Search the \_S5 package in the DSDT
	uint8_t *S5Addr =
	  (uint8_t *)(facp->Dsdt + 36 + MEM_PHYS_OFFSET); // Skip header

	printf("FADT table address: %X\n", (uintptr_t)facp);
	uint32_t offset = (uint32_t)(facp->Dsdt + MEM_PHYS_OFFSET);
	printf("FADT DSDT address: %X\n", offset);
	struct sdt *header = (struct sdt *)((uintptr_t)offset);
	int dsdtLength = header->length;
	printf("DSDT len: %X\n", dsdtLength);
	while (0 < dsdtLength--) {
		if (!memcmp((char *)S5Addr, "_S5_", 4)) {
			break;
		}
		S5Addr++;
	}

	// Check if \_S5 was found
	if (dsdtLength > 0) {
		// Check for valid AML structure
		if ((*(S5Addr - 1) == 0x08 ||
			 (*(S5Addr - 2) == 0x08 && *(S5Addr - 1) == '\\')) &&
			*(S5Addr + 4) == 0x12) {
			S5Addr += 5;
			S5Addr += ((*S5Addr & 0xC0) >> 6) + 2; // Calculate PkgLength size

			if (*S5Addr == 0x0A)
				S5Addr++; // Skip byteprefix
			SLP_TYPa = *(S5Addr) << 10;
			S5Addr++;

			if (*S5Addr == 0x0A)
				S5Addr++; // Skip byteprefix
			SLP_TYPb = *(S5Addr) << 10;

			SMI_CMD = (uint32_t *)((uintptr_t)facp->SMI_CommandPort);

			ACPI_ENABLE = facp->AcpiEnable;
			ACPI_DISABLE = facp->AcpiDisable;

			PM1a_CNT = facp->PM1aControlBlock;
			PM1b_CNT = facp->PM1bControlBlock;

			PM1_CNT_LEN = facp->PM1ControlLength;

			SLP_EN = 1 << 13;
			SCI_EN = 1;

			return;
		} else {
			printf("\\_S5 parse error\n");
		}
	} else {
		printf("ACPI: \\_S5 object not found!\n");
	}
}

void acpi_enable(void) {
	// Check if ACPI is enabled
	if ((port_word_in((unsigned short)PM1a_CNT) & SCI_EN) == 0) {
		// Check if ACPI can be enabled
		if (SMI_CMD != 0 && ACPI_ENABLE != 0) {
			port_byte_out((uint16_t)((uintptr_t)SMI_CMD),
						  ACPI_ENABLE); // Send ACPI enable command
			// Give 3 seconds time to enable ACPI
			int i;
			for (i = 0; i < 300; i++) {
				if ((port_word_in((unsigned int)PM1a_CNT) & SCI_EN) == 1)
					break;
				// TODO: Proper sleep()
				for (size_t i = 0; i < 1000000000; i++) {
				}
			}
			if (PM1b_CNT != 0) {
				for (; i < 300; i++) {
					if ((port_word_in((unsigned int)PM1b_CNT) & SCI_EN) == 1)
						break;

					// TODO: Proper sleep()
					for (size_t i = 0; i < 1000000000; i++) {
					}
				}
			}

			if (i < 300) {
				printf("Enabled ACPI\n");
			} else {
				PANIC("Couldn't enable ACPI\n");
			}
		} else {
			PANIC("Error while enabling ACPI");
		}
	}
}

void fadt_acpi_shutdown(void) {
	// SCI_EN is set to 1 if ACPI shutdown is possible
	if (SCI_EN == 0)
		return;

	// Send the shutdown command
	port_word_out((unsigned int)PM1a_CNT, SLP_TYPa | SLP_EN);
	if (PM1b_CNT != 0)
		port_word_out((unsigned int)PM1b_CNT, SLP_TYPb | SLP_EN);
}

void fadt_acpi_reboot(void) {
	if ((facp->Flags & (1 << 10)) != 0) {
		port_byte_out((unsigned short)facp->ResetReg.Address,
					  (unsigned char)facp->ResetValue);

		// If this didn't work, hard reboot
		uint8_t good = 0x02;
		while (good & 0x02)
			good = port_byte_in(0x64);
		port_byte_out(0x64, 0xFE);
	} else {
		printf("This ACPI revision doesn't support reboot");
	}
}
