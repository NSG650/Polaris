#ifndef PARTITION_H
#define PARTITION_H

#include <stddef.h>
#include <stdint.h>

struct gpt_header {
	char signature[8];
	uint32_t revision;
	uint32_t size;
	uint32_t crc32;
	uint32_t reserved;
	uint64_t headerlba;
	uint64_t alternatelba;
	uint64_t firstusable;
	uint64_t lastusable;
	uint64_t guid[2];
	uint64_t entryarraylbastart;
	uint32_t entrycount;
	uint32_t entrybytesize;
	uint32_t entryarraycrc32;
} __attribute__((packed));

struct gpt_entry {
	uint64_t typelow;
	uint64_t typehi;

	uint64_t unilow;
	uint64_t unihi;

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

#endif
