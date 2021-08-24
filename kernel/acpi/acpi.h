#ifndef ACPI_H
#define ACPI_H

/*
 * Copyright 2021 Misha
 * Copyright 2021 NSG650
 * Copyright 2021 Sebastian
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

struct GenericAddressStructure {
	uint8_t AddressSpace;
	uint8_t BitWidth;
	uint8_t BitOffset;
	uint8_t AccessSize;
	uint64_t Address;
} __attribute__((packed));

struct facp {
	struct sdt h;
	uint32_t FirmwareCtrl;
	uint32_t Dsdt;

	// Field used in ACPI 1.0; no longer in use, for compatibility only
	uint8_t Reserved;

	uint8_t PreferredPowerManagementProfile;
	uint16_t SCI_Interrupt;
	uint32_t SMI_CommandPort;
	uint8_t AcpiEnable;
	uint8_t AcpiDisable;
	uint8_t S4BIOS_REQ;
	uint8_t PSTATE_Control;
	uint32_t PM1aEventBlock;
	uint32_t PM1bEventBlock;
	uint32_t PM1aControlBlock;
	uint32_t PM1bControlBlock;
	uint32_t PM2ControlBlock;
	uint32_t PMTimerBlock;
	uint32_t GPE0Block;
	uint32_t GPE1Block;
	uint8_t PM1EventLength;
	uint8_t PM1ControlLength;
	uint8_t PM2ControlLength;
	uint8_t PMTimerLength;
	uint8_t GPE0Length;
	uint8_t GPE1Length;
	uint8_t GPE1Base;
	uint8_t CStateControl;
	uint16_t WorstC2Latency;
	uint16_t WorstC3Latency;
	uint16_t FlushSize;
	uint16_t FlushStride;
	uint8_t DutyOffset;
	uint8_t DutyWidth;
	uint8_t DayAlarm;
	uint8_t MonthAlarm;
	uint8_t Century;

	// Reserved in ACPI 1.0; used since ACPI 2.0+
	uint16_t BootArchitectureFlags;

	uint8_t Reserved2;
	uint32_t Flags;

	// Reset register
	struct GenericAddressStructure ResetReg;

	uint8_t ResetValue;
	uint8_t Reserved3[3];

	// 64-bit pointers - Available on ACPI 2.0+
	uint64_t X_FirmwareControl;
	uint64_t X_Dsdt;

	struct GenericAddressStructure X_PM1aEventBlock;
	struct GenericAddressStructure X_PM1bEventBlock;
	struct GenericAddressStructure X_PM1aControlBlock;
	struct GenericAddressStructure X_PM1bControlBlock;
	struct GenericAddressStructure X_PM2ControlBlock;
	struct GenericAddressStructure X_PMTimerBlock;
	struct GenericAddressStructure X_GPE0Block;
	struct GenericAddressStructure X_GPE1Block;
} __attribute__((packed));

void acpi_init(struct rsdp *rsdp);
void *acpi_find_sdt(const char *signature, int index);

#endif
