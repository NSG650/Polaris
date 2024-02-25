#ifndef TARFS_H
#define TARFS_H

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

struct tarfs {
	struct vfs_filesystem fs;
	struct vfs_node *device;
	uint64_t dev_id;
	uint32_t inode_count;
	uint64_t total_size;
	bool root;
	HASHMAP_TYPE(struct vfs_node *) directories;
};

struct tarfs_resource {
	struct resource res;
	struct tarfs *fs;
	off_t offset;
};

#endif