#ifndef ELF_H
#define ELF_H

#include <stddef.h>
#include <stdint.h>

struct elf_header {
	uint8_t identifier[16];

	uint16_t type;
	uint16_t machine;
	uint32_t version;

	uint64_t entry;

	uint64_t prog_head_off;
	uint64_t sect_head_off;

	uint32_t flags;

	uint16_t head_size;
	uint16_t prog_head_size;
	uint16_t prog_head_count;

	uint16_t sect_head_size;
	uint16_t sect_head_num;
	uint16_t sect_string_table;
} __attribute__((packed));

struct elf_prog_header {
	uint32_t type;
	uint32_t flags;

	uint64_t offset;
	uint64_t virt_addr;
	uint64_t phys_addr;
	uint64_t file_size;
	uint64_t mem_size;

	uint64_t reserved1;
} __attribute__((packed));

#endif
