#ifndef FAT32_H
#define FAT32_H

#include <fs/vfs.h>
#include <klibc/resource.h>
#include <stddef.h>
#include <stdint.h>

struct fat32_bpb {
	uint8_t jump[3];
	char oem[8];
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint16_t reserved_sectors;
	uint8_t fats_count;
	uint16_t root_entries_count;
	uint16_t sectors_count_16;
	uint8_t media_descriptor_type;
	uint16_t sectors_per_fat_16;
	uint16_t sectors_per_track;
	uint16_t heads_count;
	uint32_t hidden_sectors_count;
	uint32_t sectors_count_32;
	uint32_t sectors_per_fat_32;
	uint16_t flags;
	uint16_t fat_version_number;
	uint32_t root_directory_cluster;
	uint16_t fs_info_sector;
	uint16_t backup_boot_sector;
	uint8_t reserved[12];
	uint8_t drive_number;
	uint8_t nt_flags;
	uint8_t signature;
	uint32_t volume_serial_number;
	char label[11];
	char system_identifier[8];
} __attribute__((packed));

struct fat32_dir_entry {
	uint8_t name[11];
	uint8_t attributes;
	uint8_t reserved;
	uint8_t create_time_tenth;
	uint16_t creation_time;
	uint16_t creation_date;
	uint16_t access_date;
	uint16_t cluster_high;
	uint16_t last_mod_time;
	uint16_t last_mod_date;
	uint16_t cluster_low;
	uint32_t size;
} __attribute__((packed));

struct fat32_lfn_dir_entry {
	uint8_t order;
	uint16_t name1[5];
	uint8_t attribute;
	uint8_t reserved;
	uint8_t checksum;
	uint16_t name2[6];
	uint16_t zero;
	uint16_t name3[2];
} __attribute__((packed));

struct fat32_fs_info {
	uint32_t signature0;
	uint32_t free_clusters;
	uint32_t start_free_clusters;
	uint32_t reserved[3];
	uint32_t signature1;
} __attribute__((packed));

struct fat32_fs {
	struct vfs_filesystem fs;
	struct fat32_bpb bpb;
	struct fat32_fs_info fs_info;
	struct vfs_node *device;

	size_t cluster_size;
	size_t cluster_count;
	off_t fat_offset;
	off_t data_offset;
	ino_t current_inode;
};

struct fat32_resource {
	struct resource res;
	struct fat32_fs *fs;
	struct resource *mount_dir;
	off_t dir_offset;
	uint32_t cluster;
};

#define FAT32_FS_INFO_SIGNATURE_MINUS_1 0x41615252
#define FAT32_FS_INFO_SIGNATURE_0 0x61417272
#define FAT32_FS_INFO_SIGNATURE_1 0xaa550000

#define FAT32_ATTRIBUTES_READ_ONLY (1 << 0)
#define FAT32_ATTRIBUTES_HIDDEN (1 << 1)
#define FAT32_ATTRIBUTES_SYSTEM (1 << 2)
#define FAT32_ATTRIBUTES_VOLUME_ID (1 << 3)
#define FAT32_ATTRIBUTES_DIRECTORY (1 << 4)
#define FAT32_ATTRIBUTES_ARCHIVE (1 << 5)
#define FAT32_ATTRIBUTES_LFN 15

#define FAT32_FREE_CLUSTER_COUNT_CORRUPTED 0xFFFFFFFF

#define FAT32_FINAL_CLUSTER 0xffffff8
#define IS_FINAL_CLUSTER(c) (c >= FAT32_FINAL_CLUSTER)
#define DIR_GET_CLUSTER(d) \
	((uint32_t)d->cluster_low | ((uint32_t)d->cluster_high << 16))
#define DIR_SET_CLUSTER(d, val)      \
	(d)->cluster_low = val & 0xffff; \
	(d)->cluster_high = (val >> 16) & 0xffff

#endif
