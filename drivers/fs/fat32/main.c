#include "fat32.h"
#include <debug/debug.h>
#include <fs/vfs.h>
#include <klibc/mem.h>
#include <klibc/module.h>

static void fat32_update_fs_info(struct fat32_fs *fs) {
	struct resource *res = fs->device->resource;

	if (fs->fs_info.free_clusters == FAT32_FREE_CLUSTER_COUNT_CORRUPTED ||
		fs->fs_info.start_free_clusters == FAT32_FREE_CLUSTER_COUNT_CORRUPTED) {
		// Fine I'll do it myself
		size_t cluster_per_sector = res->stat.st_blksize / 4;
		fs->fs_info.free_clusters = 0;
		uint32_t *fat_buffer = kmalloc(res->stat.st_blksize);

		for (size_t sector = 0; sector < fs->bpb.sectors_per_fat_32; sector++) {
			if (res->read(res, NULL, fat_buffer,
						  fs->fat_offset + sector * res->stat.st_blksize,
						  res->stat.st_blksize) < 0) {
				kprintf("Failed to read fat sector!\n");
				break;
			}

			for (size_t cluster = 0; cluster < cluster_per_sector; cluster++) {
				if (fat_buffer[cluster] == 0) {
					fs->fs_info.start_free_clusters =
						cluster + sector * cluster_per_sector;
				}
				fs->fs_info.free_clusters++;
			}
		}

		kfree(fat_buffer);
	}

	if (res->write(res, NULL, &fs->fs_info,
				   res->stat.st_blksize * fs->bpb.fs_info_sector + 480 + 4,
				   sizeof(struct fat32_fs_info)) < 0) {
		kprintf("Failed to write updated fat32 fs info structure\n");
	}
}

static bool fat32_get_next_cluster(struct fat32_fs *fs, uint32_t *cluster) {
	struct resource *res = fs->device->resource;

	if (!cluster)
		return false;

	if (res->read(res, NULL, cluster, fs->fat_offset + *cluster * 4, 4) < 0) {
		return false;
	}
	return true;
}

static size_t fat32_get_chain_size(struct fat32_fs *fs, uint32_t *cluster) {
	size_t count = 0;

	while (!(IS_FINAL_CLUSTER(*cluster))) {
		count++;
		if (!fat32_get_next_cluster(fs, cluster)) {
			break;
		}
	}

	return count;
}

static struct fat32_fs *fat32_instantiate(void) {
	struct fat32_fs *f = kmalloc(sizeof(struct fat32_fs));
	if (f == NULL) {
		return f;
	}
	f->fs.create = NULL;
	f->fs.symlink = NULL;
	f->fs.link = NULL;

	return f;
}

static struct vfs_node *fat32_mount(struct vfs_node *parent, const char *name,
									struct vfs_node *device) {
	struct vfs_node *node = NULL;
	struct fat32_fs *fs = fat32_instantiate();

	if (fs == NULL) {
		return NULL;
	}

	int ret = device->resource->read(device->resource, NULL, &fs->bpb, 0,
									 sizeof(struct fat32_bpb));

	if (fs->bpb.signature != 0x29) {
		kprintf("fat32: bpb signature check failed\n");
		return NULL;
	}
	if (strncmp(fs->bpb.system_identifier, "FAT32   ", 8)) {
		kprintf("fat32: system identifier check failed\n");
		return NULL;
	}

	// The fat32 fs info structure actually consists of another signature as
	// well it is then followed by 480 reserved bytes. It would be wasteful to
	// include those in a structure. Here we read the first 4 bytes for the
	// first signature and skip 480 bytes ahead to read the useful info.

	uint32_t signatureminus1 = 0;
	device->resource->read(
		device->resource, NULL, &signatureminus1,
		fs->bpb.fs_info_sector * device->resource->stat.st_blksize, 4);
	if (signatureminus1 != FAT32_FS_INFO_SIGNATURE_MINUS_1) {
		kprintf("fat32: fs info signature-1 check failed\n");
		kfree(fs);
		return NULL;
	}

	device->resource->read(
		device->resource, NULL, &fs->fs_info,
		(fs->bpb.fs_info_sector * device->resource->stat.st_blksize) + 480 + 4,
		sizeof(struct fat32_fs_info));
	if (fs->fs_info.signature0 != FAT32_FS_INFO_SIGNATURE_0 ||
		fs->fs_info.signature1 != FAT32_FS_INFO_SIGNATURE_1) {
		kprintf("fat32: fs info signature0 or signature1 check failed\n");
		kfree(fs);
		return NULL;
	}

	off_t data_sector = fs->bpb.reserved_sectors +
						fs->bpb.fats_count * fs->bpb.sectors_per_fat_32;
	fs->device = device;
	fs->cluster_size =
		fs->bpb.sectors_per_cluster * device->resource->stat.st_blksize;
	fs->fat_offset =
		fs->bpb.reserved_sectors * device->resource->stat.st_blksize;
	fs->data_offset = data_sector * device->resource->stat.st_blksize;
	fs->cluster_count = (device->resource->stat.st_blocks - data_sector) /
						fs->bpb.sectors_per_cluster;

	// create the node before we touch the disk
	node = vfs_create_node((struct vfs_filesystem *)fs, parent, name, true);
	struct fat32_resource *res = resource_create(sizeof(struct fat32_resource));

	if (node == NULL || res == NULL) {
		kprintf("fat32: Failed to create resource\n");
		kfree(fs);
		return NULL;
	}

	fat32_update_fs_info(fs);

	uint32_t root_director_cluster = fs->bpb.root_directory_cluster;

	res->res.stat.st_blocks = fat32_get_chain_size(fs, &root_director_cluster);
	res->res.stat.st_size = res->res.stat.st_blocks * fs->cluster_size;
	res->res.stat.st_blksize = fs->cluster_size;
	res->res.stat.st_dev = device->resource->stat.st_rdev;
	res->res.stat.st_mode = 0644 | S_IFDIR;
	res->res.stat.st_nlink = 1;
	res->res.stat.st_ino = 2;

	res->fs = fs;
	res->cluster = fs->bpb.root_directory_cluster;

	node->resource = (struct resource *)res;
	node->filesystem = (struct vfs_filesystem *)fs;

	return node;
}

uint64_t driver_entry(struct module *driver_module) {
	driver_module->exit = NULL;

	vfs_add_filesystem(fat32_mount, "fat32");

	return 0;
}
