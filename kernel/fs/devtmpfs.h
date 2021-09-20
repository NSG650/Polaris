#ifndef DEVTMPFS_H
#define DEVTMPFS_H

#include "../klibc/resource.h"
#include "vfs.h"
#include <stdbool.h>

extern struct filesystem devtmpfs;

bool devtmpfs_add_device(struct resource *res, const char *name);

#endif
