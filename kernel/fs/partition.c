#include <debug/debug.h>
#include <errno.h>
#include <fs/devtmpfs.h>
#include <fs/partition.h>
#include <klibc/mem.h>
#include <mm/slab.h>

static ssize_t partition_read(struct resource *_this,
							  struct f_description *description, void *buf,
							  off_t loc, size_t count) {
	struct partition_device *this = (struct partition_device *)_this;
	if ((size_t)loc >= (this->sectors * this->lba_size)) {
		errno = ESPIPE;
		return -1;
	}
	return this->root->read(this->root, description, buf,
							loc + this->start * this->lba_size, count);
}

static ssize_t partition_write(struct resource *_this,
							   struct f_description *description,
							   const void *buf, off_t loc, size_t count) {
	struct partition_device *this = (struct partition_device *)_this;
	if ((size_t)loc >= (this->sectors * this->lba_size)) {
		errno = ESPIPE;
		return -1;
	}
	return this->root->write(this->root, description, buf,
							 loc + this->start * this->lba_size, count);
}

static bool partition_enumerate_gpt(struct resource *res, char *root_name) {
	uint16_t block_size = res->stat.st_blksize;
	struct gpt_header header = {0};
	off_t loc = 512;
	res->read(res, NULL, &header, loc, sizeof(struct gpt_header));
	loc += 512;
	if (strncmp(header.signature, "EFI PART", 8)) {
		return false;
	}
	if (header.size < 92) {
		return false;
	}
	if (header.size > res->stat.st_size) {
		return false;
	}
	if (header.header_lba != 1) {
		return false;
	}
	if (header.first_usable > header.last_usable) {
		kprintf("wtf?\n");
		return false;
	}
	struct gpt_entry entry = {0};
	loc = header.entry_array_lba_start * 512;
	for (uint32_t i = 0; i < header.entry_count; i++) {
		res->read(res, NULL, &entry, loc, sizeof(struct gpt_entry));
		loc += sizeof(struct gpt_entry);
		if (entry.uni_low == 0 && entry.uni_hi == 0) {
			continue;
		}
		if (entry.attr & (GPT_DONT_MOUNT | GPT_LEGACY)) {
			continue;
		}
		struct partition_device *part_dev =
			resource_create(sizeof(struct partition_device));
		part_dev->root = res;
		part_dev->lba_size = block_size;
		part_dev->start = entry.start;
		part_dev->sectors = entry.end - entry.start;
		part_dev->res.stat.st_blksize = block_size;
		part_dev->res.stat.st_size = part_dev->sectors * block_size;
		part_dev->res.stat.st_blocks = part_dev->sectors;
		part_dev->res.stat.st_mode = 0666 | S_IFBLK;
		part_dev->res.stat.st_rdev = resource_create_dev_id();
		part_dev->res.can_mmap = false;
		part_dev->res.ioctl = resource_default_ioctl;
		part_dev->res.read = partition_read;
		part_dev->res.write = partition_write;

		char name[32] = {0};
		char num[21] = {0};
		strncpy(name, root_name, 32);
		strcat(name, "p");
		ultoa(i + 1, num, 10);
		strcat(name, num);
		kprintf("%s: Starting from %u to %u\n", name, entry.start, entry.end);
		devtmpfs_add_device((struct resource *)part_dev, name);
	}

	return true;
}

static bool partition_enumerate_mbr(struct resource *res, char *root_name) {
	uint16_t mbr_magic = 0;
	uint16_t block_size = res->stat.st_blksize;

	res->read(res, NULL, &mbr_magic, 510, 2);
	if (mbr_magic != MBR_MAGIC) {
		return false;
	}
	struct mbr_entry entries[4] = {0};
	res->read(res, NULL, entries, MBR_ENTRY_OFFSET,
			  sizeof(struct mbr_entry) * 4);
	for (int i = 0; i < 4; i++) {
		if (entries[i].type == 0) {
			continue;
		}

		struct partition_device *part_dev =
			resource_create(sizeof(struct partition_device));
		part_dev->root = res;
		part_dev->lba_size = block_size;
		part_dev->start = entries[i].lba_start;
		part_dev->sectors = entries[i].lba_size;
		part_dev->res.stat.st_blksize = block_size;
		part_dev->res.stat.st_size = entries[i].lba_size * block_size;
		part_dev->res.stat.st_blocks = entries[i].lba_size;
		part_dev->res.stat.st_rdev = resource_create_dev_id();
		part_dev->res.can_mmap = false;
		part_dev->res.ioctl = resource_default_ioctl;
		part_dev->res.read = partition_read;
		part_dev->res.write = partition_write;
		part_dev->res.stat.st_mode = 0666 | S_IFBLK;

		char name[32] = {0};
		char num[21] = {0};
		strncpy(name, root_name, 32);
		strcat(name, "p");
		ultoa(i, num, 10);
		strcat(name, num);
		kprintf("%s: Starting from %u to %u\n", name, part_dev->start,
				part_dev->start + part_dev->sectors);
		devtmpfs_add_device((struct resource *)part_dev, name);
	}
	return true;
}

void partition_enumerate(struct resource *res, char *root_name) {
	if (!res)
		return;
	if (!root_name)
		return;

	if (partition_enumerate_gpt(res, root_name)) {
		kprintf("%s is a GPT drive!\n", root_name);
		return;
	}
	if (partition_enumerate_mbr(res, root_name)) {
		kprintf("%s is a MBR drive!\n", root_name);
		kprintf("Expect it to not work since I haven't tested this properly "
				":upside_down:\n");
		return;
	}
	kprintf("Lost cause: %s\n", root_name);
}
