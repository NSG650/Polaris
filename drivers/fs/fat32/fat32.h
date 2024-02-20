#ifndef FAT32_H
#define FAT32_H

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

#endif
