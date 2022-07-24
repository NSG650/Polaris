#include <debug/debug.h>
#include <fs/ramdisk.h>
#include <fs/vfs.h>
#include <mm/pmm.h>

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

void ramdisk_install(uintptr_t ramdisk_address, uint64_t ramdisk_size) {
	struct ustar_header *h = (void *)ramdisk_address;
	for (;;) {
		if (strncmp(h->signature, "ustar", 5))
			break;
		uintptr_t size = octal_to_int(h->size);

		switch (h->type) {
			case USTAR_DIRECTORY: {
				char *fun = kmalloc(256);
				memset(fun, 0, 256);
				strcat(fun, "/");
				size_t s = strlen(h->name);
				h->name[s - 1] = '\0';
				strcat(fun, h->name);
				struct fs_node *node = vfs_path_to_node(fun);
				if (node) {
					node->fs->mkdir(node, h->name);
				}
				kfree(fun);
				break;
			}
			case USTAR_REGULAR:
			case USTAR_NORMAL:
			case USTAR_CONTIGOUS: {
				char *fun = kmalloc(256);
				memset(fun, 0, 256);
				strcat(fun, "/");
				strcat(fun, h->name);
				struct fs_node *node = vfs_path_to_node(fun);
				if (node) {
					size_t token_count = 0;
					char *original_path = kmalloc(strlen(fun) + 1);
					strcpy(original_path, fun);
					char *save = fun;
					char *token;

					while ((token = strtok_r(save, "/", &save))) {
						token_count++;
					}
					save = original_path;
					char **token_list = kmalloc(sizeof(char *) * token_count);
					for (size_t i = 0; i < token_count; i++) {
						token = strtok_r(save, "/", &save);
						token_list[i] = token;
					}
					node->fs->create(node, token_list[token_count - 1]);
					struct file *a =
						node->fs->open(node, token_list[token_count - 1]);
					if (a) {
						void *buf = (void *)h + 512;
						kfree(a->data);
						a->allocated_size = size;
						a->write = 0;
						a->size = size;
						a->data = (uint8_t *)buf;
					}
					kfree(original_path);
					kfree(token_list);
				}
				kfree(fun);
				break;
			}
		}

		h = (void *)h + 512 + ALIGN_UP(size, 512);

		if ((uintptr_t)h >= ramdisk_address + ramdisk_size)
			break;
	}
	kprintf("VFS: Loaded ramdisk of size %u\n", ramdisk_size);
}
