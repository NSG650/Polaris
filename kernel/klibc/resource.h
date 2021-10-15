#ifndef RESOURCE_H
#define RESOURCE_H

#include "lock.h"
#include "types.h"
#include <stddef.h>

// This is the base class for all kernel handles
struct resource {
	size_t actual_size;

	int refcount;
	lock_t lock;

	struct stat st;

	int (*close)(struct resource *this);
	ssize_t (*read)(struct resource *this, void *buf, off_t loc, size_t count);
	ssize_t (*write)(struct resource *this, const void *buf, off_t loc,
					 size_t count);
	int (*ioctl)(struct resource *this, int request, ...);
};

void *resource_create(size_t actual_size);

#endif
