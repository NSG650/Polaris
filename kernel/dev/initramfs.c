/*
 * Copyright 2021, 2022 NSG650
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
#include "dev.h"
#include <stdint.h>

struct ustar_header {
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char checksum[8];
	char type;
	char linkname[100];
	char signature[6];
	char version[2];
	char owner[32];
	char group[32];
	char devmajor[8];
	char devminor[8];
	char prefix[155];
};

enum {
	USTAR_REGULAR = 0,
	USTAR_NORMAL = '0',
	USTAR_HARD_LINK = '1',
	USTAR_SYM_LINK = '2',
	USTAR_CHAR_DEV = '3',
	USTAR_BLOCK_DEV = '4',
	USTAR_DIRECTORY = '5',
	USTAR_FIFO = '6',
	USTAR_CONTIGOUS = '7'
};

static uint64_t octal_to_int(const char *s) {
	uint64_t ret = 0;
	while (*s) {
		ret *= 8;
		ret += *s - '0';
		s++;
	}
	return ret;
}

void initramfs_init(struct stivale2_struct_tag_modules *modules_tag) {
	if (modules_tag->module_count < 1) {
		PANIC("No initramfs found!");
	}

	struct stivale2_module *module = &modules_tag->modules[0];

	uintptr_t initramfs_addr = module->begin;
	uint64_t initramfs_size = module->end - module->begin;

	printf("Initramfs: Address: %p\n", initramfs_addr);
	printf("Initramfs: Size: %lld\n", initramfs_size);

	struct ustar_header *h = (void *)initramfs_addr;
	for (;;) {
		if (strncmp(h->signature, "ustar", 5))
			break;
		uintptr_t size = octal_to_int(h->size);

		switch (h->type) {
			case USTAR_DIRECTORY: {
				vfs_mkdir(NULL, h->name, octal_to_int(h->mode) & 0777, false);
				break;
			}
			case USTAR_REGULAR:
			case USTAR_NORMAL:
			case USTAR_CONTIGOUS: {
				struct resource *r =
					vfs_open(h->name, O_WRONLY | O_CREAT | O_TRUNC,
							 octal_to_int(h->mode) & 0777);
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
