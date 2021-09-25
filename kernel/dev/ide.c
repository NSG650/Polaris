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

#include "ide.h"
#include "../cpu/ports.h"
#include "../klibc/alloc.h"
#include "../klibc/mem.h"
#include "../klibc/printf.h"
#include "../sys/hpet.h"
#include "../sys/pci.h"
#include "idedef.h"
#include <stdbool.h>
#include <stdint.h>

unsigned char ide_buf[2048] = {0};
unsigned char ide_irq_invoked = 0;
unsigned char atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
struct ide_device *ide_devices[4];

inline void Theinsl(uint32_t port, void *addr, int cnt) {
	asm volatile("cld;"
				 "repne; insl;"
				 : "=D"(addr), "=c"(cnt)
				 : "d"(port), "0"(addr), "1"(cnt)
				 : "memory", "cc");
}

void ata_io_wait(uint16_t bus) {
	port_byte_in(bus + ATA_REG_ALTSTATUS);
	port_byte_in(bus + ATA_REG_ALTSTATUS);
	port_byte_in(bus + ATA_REG_ALTSTATUS);
	port_byte_in(bus + ATA_REG_ALTSTATUS);
}

int ata_wait(uint16_t bus, int advanced) {
	uint8_t status = 0;

	ata_io_wait(bus);

	while ((status = port_byte_in(bus + ATA_REG_STATUS)) & ATA_SR_BSY)
		;

	if (advanced) {
		status = port_byte_in(bus + ATA_REG_STATUS);
		if (status & ATA_SR_ERR)
			return 1;
		if (status & ATA_SR_DF)
			return 1;
		if (!(status & ATA_SR_DRQ))
			return 1;
	}

	return 0;
}

void ata_select(uint16_t bus, bool Master) {

	if (Master) {
		port_byte_out(bus + ATA_REG_HDDEVSEL, 0xA0);
	} else {
		port_byte_out(bus + ATA_REG_HDDEVSEL, 0xB0);
	}
}

void ata_wait_ready(uint16_t bus) {
	while (port_byte_in(bus + ATA_REG_STATUS) & ATA_SR_BSY)
		;
}

void ide_get_string(char *stringPointer, size_t length) {
	char ptr[40];
	memcpy(ptr, stringPointer, length);

	for (size_t i = 0; i < length - 1; i += 2) {
		uint8_t tmp = ptr[i + 1];
		ptr[i + 1] = ptr[i];
		ptr[i] = tmp;
	}
	memcpy(stringPointer, ptr, length);
}

struct ide_device *ide_device_init(bool Primary, bool Master) {
	uint16_t bus;
	if (Primary) {
		bus = 0x1F0;
	} else {
		bus = 0x170;
	}

	struct ide_device *atadevice = alloc(sizeof(struct ide_device));
	atadevice->exists = 0;
	atadevice->channel = Primary ? 0 : 1;
	atadevice->drive = Master ? 0 : 1;
	port_byte_out(bus + 1, 1);
	port_byte_out(bus + 0x306, 0);

	ata_select(bus, Master);
	ata_io_wait(bus);

	if (port_byte_in(bus + ATA_REG_STATUS) == 0xFF) {
		// Floating device
		return atadevice;
	}

	printf("Device status: %X\n", port_byte_in(bus + ATA_REG_STATUS));
	uint8_t deviceStatus = (uint8_t)port_byte_in(bus + ATA_REG_STATUS);
	if (deviceStatus == 0) {
		// Non existant device
		return atadevice;
	} else if (deviceStatus & ATA_SR_ERR) {
		uint32_t type_id = (port_byte_in(bus + ATA_REG_LBA2) << 8) |
						   port_byte_in(bus + ATA_REG_LBA1);
		if (type_id == 0xEB14 || type_id == 0x9669) {
			// ATAPI device
			atadevice->type = 1;
			printf("ATAPI device\n");
		} else {
			// Not a device
			printf("Invaild device\n");
			return atadevice;
		}
	} else if (deviceStatus & ATA_SR_DRQ) {
		// Error
		return atadevice;
	}

	atadevice->exists = 1;

	port_byte_out(bus + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
	ata_io_wait(bus);

	struct ata_identify_t *device;
	uint16_t *buf = alloc(512);

	for (int i = 0; i < 256; ++i) {
		buf[i] = port_word_in(bus);
		// printf("%X,", buf[i]);
	}
	// printf("\n");

	device = (struct ata_identify_t *)buf;

	ata_wait_ready(bus);
	ide_get_string(device->model, 40);
	ide_get_string(device->firmware, 8);
	ide_get_string(device->serial, 20);

	printf("ATA device model: %s\n", device->model);
	printf("sectors_48 = %X\n", (uint32_t)device->sectors_48);
	printf("sectors_28 = %X\n", device->sectors_28);

	// port_byte_out(bus + ATA_REG_CONTROL, 0x02);
	return atadevice;
}

void ide_init(void) {
	struct pci_device *ideDrive;
	for (size_t i = 0; i < 100; i++) {
		struct pci_device *dev = PCIDevicesArray[i];
		if (dev != NULL) {
			if (dev->classCode == 0x1 && dev->subclass == 0x01) {
				ideDrive = dev;
				break;
			}
		}
	}
	if (ideDrive == NULL) {
		printf("IDE: NO Ide controller found\n");
		return;
	} else {
		printf("Found IDE controller\n");
	}
	ide_devices[0] = ide_device_init(true, true);  // Primary master
	ide_devices[1] = ide_device_init(true, false); // Primary slave

	ide_devices[2] = ide_device_init(false, true);	// Secondary master
	ide_devices[3] = ide_device_init(false, false); // Secondary slave
}
