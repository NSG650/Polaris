#ifndef RESOURCE_H
#define RESOURCE_H

/*
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

#include "lock.h"
#include "types.h"
#include <stddef.h>

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
