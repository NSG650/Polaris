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

struct ide_device *ide_devices[4];

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
			printf("ide: found ATAPI device\n");
		} else {
			// Not a device
			printf("ide: found an Invaild device\n");
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
	}

	device = (struct ata_identify_t *)buf;

	ata_wait_ready(bus);
	ide_get_string(device->model, 40);
	ide_get_string(device->firmware, 8);
	ide_get_string(device->serial, 20);

	printf("ide: ATA device model: %s\n", device->model);

	// port_byte_out(bus + ATA_REG_CONTROL, 0x02);
	return atadevice;
}

void ide_read_sector(uint32_t device_index, uint32_t lba, uint8_t *buffer) {
	struct ide_device *device = ide_devices[device_index];
	if (device->exists == 0) {
		printf("ERROR: ide_read_sector() called with a device that does NOT "
			   "exist\n");
		return;
	}
	if (device->type == 1) {
		printf("ERROR: ide_read_sector() called with a device that is a ATAPI "
			   "device, which is not yet supported!\n");
		return;
	}

	uint16_t bus;
	if (device->channel == 0) {
		bus = 0x1F0;
	} else {
		bus = 0x170;
	}
	port_byte_out(bus + ATA_REG_CONTROL, 0x02); // PIO mode
	// Set device
	ata_wait_ready(bus);
	port_byte_out(bus + ATA_REG_HDDEVSEL,
				  0xe0 | device->channel << 4 | (lba & 0x0f000000) >> 24);

	port_byte_out(bus + ATA_REG_FEATURES, 0x00);
	port_byte_out(bus + ATA_REG_SECCOUNT0, 1);
	port_byte_out(bus + ATA_REG_LBA0, (lba & 0x000000ff) >> 0);
	port_byte_out(bus + ATA_REG_LBA1, (lba & 0x0000ff00) >> 8);
	port_byte_out(bus + ATA_REG_LBA2, (lba & 0x00ff0000) >> 16);
	port_byte_out(bus + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

	if (ata_wait(bus, 1)) {
		printf("ide: Error during ATA read");
	}
	int size = 256;

	for (int i = 0; i < size; ++i) {
		buffer[i] = port_word_in(bus);
	}
	ata_wait(bus, 0);
}

void ide_write_sector(uint32_t device_index, uint32_t lba, uint8_t *buffer) {
	struct ide_device *device = ide_devices[device_index];
	if (device->exists == 0) {
		printf("ERROR: ide_write_sector() called with a device that does NOT "
			   "exist\n");
		return;
	}
	if (device->type == 1) {
		printf("ERROR: ide_write_sector() called with a device that is a ATAPI "
			   "device, which is not yet supported!\n");
		return;
	}

	uint16_t bus;
	if (device->channel == 0) {
		bus = 0x1F0;
	} else {
		bus = 0x170;
	}
	port_byte_out(bus + ATA_REG_CONTROL, 0x02);

	ata_wait_ready(bus);

	port_byte_out(bus + ATA_REG_HDDEVSEL,
				  0xe0 | device->channel << 4 | (lba & 0x0f000000) >> 24);
	ata_wait(bus, 0);
	port_byte_out(bus + ATA_REG_FEATURES, 0x00);
	port_byte_out(bus + ATA_REG_SECCOUNT0, 0x01);
	port_byte_out(bus + ATA_REG_LBA0, (lba & 0x000000ff) >> 0);
	port_byte_out(bus + ATA_REG_LBA1, (lba & 0x0000ff00) >> 8);
	port_byte_out(bus + ATA_REG_LBA2, (lba & 0x00ff0000) >> 16);
	port_byte_out(bus + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
	ata_wait(bus, 0);

	//Send the new sector
	uint16_t *buffer2 = (uint16_t *)buffer;
	for (size_t i = 0; i < 256; i++) {
		port_word_out(bus, buffer2[i]);
	}

	port_byte_out(bus + 0x07, ATA_CMD_CACHE_FLUSH);
	ata_wait(bus, 0);
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
		printf("ide: NO IDE controller found\n");
		return;
	} else {
		printf("ide: Found IDE controller\n");
	}
	ide_devices[0] = ide_device_init(true, true);  // Primary master
	ide_devices[1] = ide_device_init(true, false); // Primary slave

	ide_devices[2] = ide_device_init(false, true);	// Secondary master
	ide_devices[3] = ide_device_init(false, false); // Secondary slave

	ide_read_sector(0, 0, alloc(512));
	ide_read_sector(1, 0, alloc(512));
	ide_read_sector(2, 0, alloc(512));
	ide_read_sector(3, 0, alloc(512));
}
