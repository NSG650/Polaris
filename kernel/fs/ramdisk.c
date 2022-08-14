#include <debug/debug.h>
#include <fs/ramdisk.h>
#include <fs/vfs.h>
#include <klibc/misc.h>

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
	USTAR_CONTIGOUS = '7',
	USTAR_GNU_LONG_PATH = 'L'
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

void ramdisk_install(uintptr_t ramdisk_address, uint64_t ramdisk_size) {
	struct ustar_header *current_file = (void *)ramdisk_address;
	char *name_override = NULL;

	while (strncmp(current_file->signature, "ustar", 5) == 0) {
		char *name = current_file->name;
		if (name_override != NULL) {
			name = name_override;
			name_override = NULL;
		}

		if (strcmp(name, "./") == 0)
			continue;

		size_t mode = octal_to_int(current_file->mode);
		size_t size = octal_to_int(current_file->size);

		struct vfs_node *node = NULL;
		switch (current_file->type) {
			case USTAR_REGULAR:
			case USTAR_NORMAL:
			case USTAR_CONTIGOUS: {
				node = vfs_create(vfs_root, name, mode | S_IFREG);
				if (node) {
					struct resource *resource = node->resource;
					resource->write(resource, NULL, (void *)current_file + 512,
									0, size);
				}
				break;
			}
			case USTAR_SYM_LINK: {
				vfs_symlink(vfs_root, current_file->linkname, name);
				break;
			}
			case USTAR_DIRECTORY: {
				vfs_create(vfs_root, name, mode | S_IFDIR);
				break;
			}
			case USTAR_GNU_LONG_PATH: {
				name_override = (void *)current_file + 512;
				name_override[size] = 0;
				break;
			}
		}

		current_file = (void *)current_file + 512 + ALIGN_UP(size, 512);
	}

	kprintf("VFS: Loaded ramdisk of size %u\n", ramdisk_size);
}
