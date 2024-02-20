#ifndef PARTITION_H
#define PARTITION_H

#include <klibc/resource.h>
#include <stddef.h>
#include <stdint.h>

struct gpt_header {
	char signature[8];
	uint32_t revision;
	uint32_t size;
	uint32_t crc32;
	uint32_t reserved;
	uint64_t header_lba;
	uint64_t alternate_lba;
	uint64_t first_usable;
	uint64_t last_usable;
	uint64_t guid[2];
	uint64_t entry_array_lba_start;
	uint32_t entry_count;
	uint32_t entry_byte_size;
	uint32_t entry_array_crc32;
} __attribute__((packed));

struct gpt_entry {
	uint64_t type_low;
	uint64_t type_hi;

	uint64_t uni_low;
	uint64_t uni_hi;

	uint64_t start;
	uint64_t end;

	uint64_t attr;

	uint16_t name[36]; // the name is WCHAR
} __attribute__((packed));

struct mbr_entry {
	uint8_t status;
	uint8_t start[3];
	uint8_t type;
	uint8_t end[3];
	uint32_t lba_start;
	uint32_t lba_size;
} __attribute__((packed));

struct partition_device {
	struct resource res;
	uint64_t start;
	uint64_t sectors;
	uint16_t lba_size;
	struct resource *root;
};

#define MBR_MAGIC 0xaa55
#define MBR_ENTRY_OFFSET 510

#define GPT_IMPORTANT 1
#define GPT_DONT_MOUNT 2
#define GPT_LEGACY 4

void partition_enumerate(struct resource *res, char *root_name);

#endif
