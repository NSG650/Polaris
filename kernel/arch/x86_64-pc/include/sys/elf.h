#ifndef _SYS__ELF_H
#define _SYS__ELF_H

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

#endif
