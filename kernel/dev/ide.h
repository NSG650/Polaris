#ifndef IDE_H
#define IDE_H
#include <stdint.h>
extern struct ide_device *ide_devices[4];
void ide_init();
void ide_read_sector(uint32_t device_index, uint32_t lba, uint8_t *buffer);
void ide_write_sector(uint32_t device_index, uint32_t lba, uint8_t *buffer);
#endif
