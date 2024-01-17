#ifndef RAMDISK_H
#define RAMDISK_H

#include <stddef.h>
#include <stdint.h>

void ramdisk_install(uintptr_t ramdisk_address, uint64_t ramdisk_size);

#endif
