/*
 * Copyright 2021 NSG650
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

#include "initramfs.h"
#include "../fs/vfs.h"
#include "../kernel/panic.h"
#include "../klibc/math.h"
#include "../klibc/printf.h"
#include "../klibc/string.h"
#include "../mm/vmm.h"
#include "dev.h"
#include <stddef.h>
#include <stdint.h>
#include <stivale2.h>

struct ustar_header {
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char checksum[8];
	uint8_t type;
	char link_name[100];
	char signature[6];
	char version[2];
	char owner[32];
	char group[32];
	char device_maj[8];
	char device_min[8];
	char prefix[155];
};

enum {
	USTAR_FILE = '0',
	USTAR_HARD_LINK = '1',
	USTAR_SYM_LINK = '2',
	USTAR_CHAR_DEV = '3',
	USTAR_BLOCK_DEV = '4',
	USTAR_DIR = '5',
	USTAR_FIFO = '6'
};

static uintptr_t initramfs_addr;
static uintptr_t initramfs_size;

static uint64_t octal_to_int(const char *s) {
	uint64_t ret = 0;
	while (*s) {
		ret *= 8;
		ret += *s - '0';
		s++;
	}
	return ret;
}

bool initramfs_init(struct stivale2_struct_tag_modules *modules_tag) {
	if (modules_tag->module_count < 1) {
		PANIC("No initramfs found!");
	}

	struct stivale2_module *module = &modules_tag->modules[0];

	initramfs_addr = module->begin;
	initramfs_size = module->end - module->begin;

	printf("initramfs: Address: %p\n", initramfs_addr);
	printf("initramfs: Size:    %d\n", initramfs_size);

	struct ustar_header *h = (void *)initramfs_addr;
	for (;;) {
		if (strncmp(h->signature, "ustar", 5))
			break;
		uintptr_t size = octal_to_int(h->size);

		switch (h->type) {
			case USTAR_DIR: {
				vfs_mkdir(NULL, h->name, octal_to_int(h->mode), false);
				break;
			}
			case USTAR_FILE: {
				struct resource *r =
					vfs_open(h->name, O_RDWR | O_CREAT, octal_to_int(h->mode));
				void *buf = (void *)h + 512;
				r->write(r, buf, 0, size);
				r->close(r);
				break;
			}
		}

		h = (void *)h + 512 + ALIGN_UP(size, 512);

		if ((uintptr_t)h >= initramfs_addr + initramfs_size)
			break;
	}

	printf("initramfs: Loaded into VFS\n");
}
