#ifndef _SYS__ELF_H
#define _SYS__ELF_H

#include <klibc/elf.h>
#include <klibc/resource.h>
#include <mm/vmm.h>
#include <stdbool.h>
#include <stdint.h>

struct auxval {
	uint64_t at_entry;
	uint64_t at_phdr;
	uint64_t at_phent;
	uint64_t at_phnum;
};

bool elf_load(struct pagemap *pagemap, struct resource *res, uint64_t load_base,
			  struct auxval *auxv, const char **ld_path);

void elf_init_function_table(uint8_t *binary);
const char *elf_get_name_from_function(uint64_t address);
uint64_t elf_get_function_from_name(const char *string);

#endif
