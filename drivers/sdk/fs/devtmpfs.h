#ifndef DEVTMPFS_H
#define DEVTMPFS_H

#include <klibc/resource.h>
#include <stdbool.h>

void devtmpfs_init(void);
bool devtmpfs_add_device(struct resource *device, const char *name);

#endif
