#include "fat32.h"
#include <debug/debug.h>
#include <errno.h>
#include <fs/vfs.h>
#include <klibc/mem.h>
#include <klibc/misc.h>
#include <klibc/module.h>
#include <mm/pmm.h>
#include <mm/vmm.h>

void *memcpy(void *dest, const void *src, size_t n) {
	uint8_t *pdest = (uint8_t *)dest;
	const uint8_t *psrc = (const uint8_t *)src;

	for (size_t i = 0; i < n; i++) {
		pdest[i] = psrc[i];
	}

	return dest;
}

void *memset(void *b, int c, size_t len) {
	size_t i = 0;
	unsigned char *p = b;
	while (len > 0) {
		*p = c;
		p++;
		len--;
	}
	return b;
}

static inline bool isalphabet(char c) {
	return ((c > 64 && c < 91) || (c > 96 && c < 123));
}

static inline char tolower(char c) {
	if (isalphabet(c)) {
		c |= 0x20;
	}
	return c;
}

// I lost my braincells in this
static char *fat32_get_resonable_name(struct fat32_dir_entry *entry,
									  char *str) {
	strncpy(str, entry->name, 11);
	for (int i = 0; i < 11; i++) {
		str[i] = tolower(str[i]);
	}
	str[11] = str[10];
	str[10] = str[9];
	str[9] = str[8];

	// get rid of the spaces
	off_t offset = 7;
	while (str[offset] == ' ') {
		offset--;
	}
	// extension is empty? lets end it here
	if (str[9] == ' ') {
		str[offset + 1] = '\0';
		return str;
	}
	str[offset + 1] = '.';
	for (int i = 2; i < 5; i++) {
		// copy the extension and remove the space in the end
		if (str[i + 7] == ' ') {
			str[offset + i] = '\0';
			return str;
		}
		str[offset + i] = str[i + 7];
	}
	str[offset + 5] = '\0';
	return str;
}

static void fat32_get_string_from_lfn(struct fat32_lfn_dir_entry *entry,
									  char *str) {
	for (int i = 0; i < 5; ++i) {
		*str++ = entry->name1[i];
	}
	for (int i = 0; i < 6; ++i) {
		*str++ = entry->name2[i];
	}
	for (int i = 0; i < 2; ++i) {
		*str++ = entry->name3[i];
	}
}

// checksum function was ripped from Microsoft Extensible Firmware Initiative
// FAT32 File System Specification

static uint8_t fat32_lfn_checksum(char *short_name) {
	uint8_t sum = 0;
	for (int i = 11; i != 0; i--) {
		sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *short_name++;
	}
	return sum;
}

static inline off_t fat32_cluster_to_disk_offset(struct fat32_fs *fs,
												 uint32_t cluster) {
	return fs->data_offset + fs->cluster_size * (cluster - 2);
}

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
				kprintf("fat32: Failed to read fat sector!\n");
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
		kprintf("fat32: Failed to write updated fat32 fs info structure\n");
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

static uint32_t fat32_get_next_free_cluster(struct fat32_fs *fs,
											uint32_t cluster) {
	while (cluster) {
		uint32_t this = cluster;
		if (fat32_get_next_cluster(fs, &this) == false || this == 0) {
			break;
		}
		cluster++;
	}
	return cluster;
}

static uint32_t fat32_skip(struct fat32_fs *fs, uint32_t cluster, size_t count,
						   bool *end) {
	*end = false;
	uint32_t next = 0;
	for (size_t i = 0; i < count; i++) {
		fs->device->resource->read(fs->device->resource, NULL, &next,
								   fs->fat_offset + cluster * 4, 4);
		if (IS_FINAL_CLUSTER(next)) {
			*end = true;
			break;
		}
		cluster = next;
	}
	return cluster;
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

static ssize_t fat32_clusters_read_or_write(struct fat32_fs *fs, void *buffer,
											uint32_t cluster, size_t count,
											uint32_t *ending_cluster, bool rw) {
	size_t i = 0;
	for (i = 0; i < count; i++) {
		if (!cluster) {
			return -1;
		}
		if (IS_FINAL_CLUSTER(cluster)) {
			break;
		}
		if (cluster > fs->cluster_count) {
			return -1;
		}

		off_t disk_offset = fat32_cluster_to_disk_offset(fs, cluster);
		ssize_t status = -1;
		if (rw) {
			status = fs->device->resource->write(
				fs->device->resource, NULL,
				(void *)((uintptr_t)buffer + fs->cluster_size * i), disk_offset,
				fs->cluster_size);
		} else {
			status = fs->device->resource->read(
				fs->device->resource, NULL,
				(void *)((uintptr_t)buffer + fs->cluster_size * i), disk_offset,
				fs->cluster_size);
		}

		if (status < 0) {
			return -1;
		}
		if (!fat32_get_next_cluster(fs, &cluster)) {
			return -1;
		}
	}

	if (ending_cluster) {
		*ending_cluster = cluster;
	}

	return i;
}

static ssize_t fat32_cluster_read_or_write(struct fat32_fs *fs, void *buffer,
										   uint32_t cluster, off_t offset,
										   size_t count, bool write) {
	return write
			   ? fs->device->resource->write(
					 fs->device->resource, NULL, buffer,
					 fat32_cluster_to_disk_offset(fs, cluster) + offset, count)
			   : fs->device->resource->read(
					 fs->device->resource, NULL, buffer,
					 fat32_cluster_to_disk_offset(fs, cluster) + offset, count);
}

static bool fat32_bytes_read_or_write(struct fat32_fs *fs, uint32_t cluster,
									  void *buffer, off_t offset, size_t count,
									  bool write) {
	bool end = false;
	cluster = fat32_skip(fs, cluster, offset / fs->cluster_size, &end);

	// r/w the first cluster
	off_t cluster_offset = offset % fs->cluster_size;
	if (cluster_offset > 0) {
		size_t rcount = count > fs->cluster_size
							? fs->cluster_size - cluster_offset
							: count;
		if (fat32_cluster_read_or_write(fs, buffer, cluster, cluster_offset,
										rcount, write) < 0) {
			crash_or_not();
			return false;
		}
		fat32_get_next_cluster(fs, &cluster);
		buffer = (void *)((uintptr_t)buffer + rcount);
		count -= rcount;
	}

	// r/w the middle of the chain
	size_t clustersremaining = count / fs->cluster_size;
	if (clustersremaining &&
		fat32_clusters_read_or_write(fs, buffer, cluster, clustersremaining,
									 &cluster, write) < 0) {
		return false;
	}

	// r/w the final cluster
	count -= clustersremaining * fs->cluster_size;
	if (count > 0) {
		buffer =
			(void *)((uintptr_t)buffer + clustersremaining * fs->cluster_size);
		if (fat32_cluster_read_or_write(fs, buffer, cluster, 0, count, write) <
			0) {
			crash_or_not();
			return false;
		}
	}

	return true;
}

static ssize_t fat32_res_read(struct resource *_this,
							  struct f_description *desc, void *buffer,
							  off_t offset, size_t count) {
	struct fat32_resource *this = (struct fat32_resource *)_this;
	spinlock_acquire_or_wait(&this->res.lock);

	off_t endoffset = offset + count;

	if (endoffset > this->res.stat.st_size) {
		endoffset = this->res.stat.st_size;
		count = offset >= endoffset ? 0 : endoffset - offset;
	}

	if (count == 0) {
		spinlock_drop(&this->res.lock);
		return count;
	}

	if (fat32_bytes_read_or_write(this->fs, this->cluster, buffer, offset,
								  count, false) == false) {
		count = -1;
	}

	spinlock_drop(&this->res.lock);
	return count;
}

static ssize_t fat32_res_write(struct resource *_this,
							   struct f_description *desc, const void *buffer,
							   off_t offset, size_t count) {
	errno = EINVAL;
	return -1;
}

static bool fat32_res_truncate(struct resource *_this,
							   struct f_description *description,
							   size_t length) {
	errno = EINVAL;
	return false;
}

static void *fat32_res_mmap(struct resource *_this, size_t file_page,
							int flags) {
	struct fat32_resource *this = (struct fat32_resource *)_this;
	void *ret = NULL;

	ret = pmm_allocz(1);
	if (ret == NULL) {
		return NULL;
	}

	if (this->res.read(_this, NULL, (void *)((uintptr_t)ret + MEM_PHYS_OFFSET),
					   file_page * PAGE_SIZE, PAGE_SIZE) == -1) {
		pmm_free(ret, 1);
		return NULL;
	}

	return ret;
}

static bool fat32_res_unref(struct resource *_this,
							struct f_description *description) {
	(void)description;

	struct fat32_resource *this = (struct fat32_resource *)_this;
	spinlock_acquire_or_wait(&this->res.lock);

	this->res.refcount--;

	spinlock_drop(&this->res.lock);
	return true;
}

static struct vfs_node *fat32_create(struct vfs_filesystem *_this,
									 struct vfs_node *parent, const char *name,
									 int mode) {
	struct fat32_fs *this = (struct fat32_fs *)_this;
	size_t length = strlen(name);

	if (length > 255) {
		errno = ENAMETOOLONG;
		return NULL;
	}

	if (!S_ISDIR(mode) && !S_ISREG(mode)) {
		errno = EPERM;
		return NULL;
	}

	struct fat32_resource *res = resource_create(sizeof(struct fat32_resource));
	struct vfs_node *node = vfs_create_node(_this, parent, name, S_ISDIR(mode));

	if (node == NULL || res == NULL) {
		return NULL;
	}

	if (S_ISREG(mode)) {
		res->res.can_mmap = true;
	}

	res->res.read = fat32_res_read;
	res->res.write = fat32_res_write;
	res->res.truncate = fat32_res_truncate;
	res->res.mmap = fat32_res_mmap;
	res->res.unref = fat32_res_unref;

	res->res.stat.st_uid = 0;
	res->res.stat.st_gid = 0;

	res->res.stat.st_size = S_ISDIR(mode) ? this->cluster_size : 0;
	res->res.stat.st_blocks = S_ISDIR(mode) ? 1 : 0;
	res->res.stat.st_blksize = this->cluster_size;
	res->res.stat.st_dev = this->device->resource->stat.st_rdev;
	res->res.stat.st_mode = mode;
	res->res.stat.st_nlink = 1;
	res->res.stat.st_ino = this->current_inode++;
	res->res.refcount = 1;

	res->mount_dir = parent->resource;
	res->fs = this;
	res->dir_offset = -1;

	node->filesystem = _this;
	node->resource = (struct resource *)res;

	res->cluster = 0;

	return node;
}

static void fat32_populate(struct vfs_filesystem *_this,
						   struct vfs_node *node) {
	struct fat32_fs *this = (struct fat32_fs *)_this;
	struct fat32_resource *res = (struct fat32_resource *)(node->resource);

	struct fat32_dir_entry *pluhhh = kmalloc(res->res.stat.st_size);
	if (!pluhhh) {
		return;
	}

	if (fat32_clusters_read_or_write(this, pluhhh, res->cluster,
									 res->res.stat.st_blocks, NULL,
									 false) < 0) {
		kfree(pluhhh);
		kprintf("fat32: failed to read clusters\n");
		return;
	}

	size_t entry_count = res->res.stat.st_size / sizeof(struct fat32_dir_entry);
	bool is_lfn = false;

	char *name_buffer = kmalloc(256);
	if (!name_buffer) {
		kfree(pluhhh);
		return;
	}
	for (size_t i = 0; i < entry_count; i++) {
		uint8_t checksum = 0;
		struct fat32_dir_entry *entry = &pluhhh[i];
		if (entry->name[0] == '\0') { // end of a dir
			break;
		}

		if (entry->name[0] == 0xe5) {
			continue;
		}

		if (entry->attributes & FAT32_ATTRIBUTES_LFN) {
			struct fat32_lfn_dir_entry *lfn =
				(struct fat32_lfn_dir_entry *)entry;
			lfn->order &= 0x1f;

			if (is_lfn == false) {
				// get directory entry these LFNs belong to for checksum
				// calculation
				struct fat32_dir_entry *dent = entry + lfn->order;
				checksum = fat32_lfn_checksum(dent->name);
				is_lfn = true;
			} else if (lfn->checksum != checksum) {
				kprintf("fat32: warning bad checksum for this file.\n");
			}
			fat32_get_string_from_lfn(lfn, name_buffer + (lfn->order - 1) * 13);
			continue;
		}

		if ((entry->attributes & FAT32_ATTRIBUTES_VOLUME_ID)) {
			continue;
		}

		if (is_lfn == false) {
			fat32_get_resonable_name(entry, name_buffer);
		}

		is_lfn = false;

		if (!strcmp(name_buffer, ".") || !strcmp(name_buffer, "..")) {
			continue;
		}

		uint16_t mode =
			0751 | ((entry->attributes & FAT32_ATTRIBUTES_DIRECTORY) ? S_IFDIR
																	 : S_IFREG);

		struct fat32_resource *res =
			resource_create(sizeof(struct fat32_resource));

		if (!res) {
			continue;
		}

		struct vfs_node *fnode = vfs_create_node(
			(struct vfs_filesystem *)this, node, name_buffer, S_ISDIR(mode));

		if (!fnode) {
			kfree(res);
			continue;
		}

		if (S_ISREG(mode)) {
			res->res.can_mmap = true;
		}

		res->res.read = fat32_res_read;
		res->res.write = fat32_res_write;
		res->res.truncate = fat32_res_truncate;
		res->res.mmap = fat32_res_mmap;
		res->res.unref = fat32_res_unref;

		res->cluster = DIR_GET_CLUSTER(entry);
		uint32_t cluster = res->cluster;

		res->res.stat.st_uid = 0;
		res->res.stat.st_gid = 0;
		res->res.stat.st_mode = mode;
		res->res.stat.st_ino = this->current_inode++;
		res->res.stat.st_blksize = this->cluster_size;
		res->res.stat.st_size = S_ISDIR(mode)
									? fat32_get_chain_size(this, &cluster) *
										  res->res.stat.st_blksize
									: entry->size;
		res->res.stat.st_blocks =
			DIV_ROUNDUP(res->res.stat.st_size, this->cluster_size);
		res->res.stat.st_nlink = 1;

		res->res.refcount = 1;
		res->dir_offset = (uintptr_t)entry - (uintptr_t)pluhhh;
		res->mount_dir = (struct resource *)res;
		res->fs = this;

		fnode->filesystem = _this;
		fnode->resource = (struct resource *)res;

		HASHMAP_SINSERT(&fnode->parent->children, name_buffer, fnode);

		if (S_ISDIR(mode)) {
			fnode->populated = false;
			vfs_create_dotentries(fnode, fnode->parent);
		}
	}

	node->populated = true;

	kfree(name_buffer);
	kfree(pluhhh);
}

static struct fat32_fs *fat32_instantiate(void) {
	struct fat32_fs *f = kmalloc(sizeof(struct fat32_fs));
	if (f == NULL) {
		return f;
	}

	f->fs.create = fat32_create;
	f->fs.populate = fat32_populate;

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
	fs->current_inode = 3;

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

	res->res.read = fat32_res_read;
	res->res.mmap = fat32_res_mmap;

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
