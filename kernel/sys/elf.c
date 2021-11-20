#include "elf.h"
#include "../fs/vfs.h"
#include "../klibc/mem.h"
#include "../klibc/printf.h"
#include "../klibc/string.h"
#include "../sched/process.h"
#include <liballoc.h>
#include <stddef.h>

static const char *ELF_SECTION_TYPE[] = {
	"NULL",	   "PROGBITS", "SYMTAB", "STRTAB", "RELA", "HASH",
	"DYNAMIC", "NOTE",	   "NOBITS", "REL",	   "",	   "DYNSYM"};

void dump_section_table(struct elf64_section_header *elf_section_headers,
						size_t section_count, char *section_strtab) {
	printf("IND |                 NAME |     TYPE | FLAG | OFFSET |   SIZE | "
		   "ADDRESS          |\n");
	printf("----+----------------------+----------+------+--------+--------+---"
		   "---------------+\n");
	for (size_t index = 0; index < section_count; index++) {
		struct elf64_section_header *section = (elf_section_headers + index);
		char *name = (section_strtab + section->name_index);

		printf("%3zu | ", index);
		printf("%20s | ", name);
		printf("%8s | ", ELF_SECTION_TYPE[section->type]);
		printf("%c%c%c%c | ", (section->flags & ELF_FLAG_WRITE) ? 'W' : ' ',
			   (section->flags & ELF_FLAG_ALLOC) ? 'A' : ' ',
			   (section->flags & ELF_FLAG_EXEC) ? 'X' : ' ',
			   (section->flags & ~0x7) ? '?' : ' ');
		printf("%6llx | ", section->offset);
		printf("%6llx | ", section->size);
		printf("%.16llx | ", section->address);
		printf("\n");
	}
}

int load_elf(char *file_name) {
	struct resource *res = vfs_open(file_name, O_RDONLY, 0644);

	struct elf64_header *elf_header = kmalloc(sizeof(struct elf64_header));
	res->read(res, elf_header, 0, sizeof(struct elf64_header));

	if (elf_header->magic != 0x464C457F) {
		printf("Invalid ELF header!\n");
		return 1;
	}

	if (elf_header->file_class != ELF_CLASS_64 ||
		elf_header->encoding != ELF_DATA_LITTLE) {
		printf("This file isn't 64-bit little endian\n");
		return 1;
	}

	if (elf_header->file_type != ELF_FILE_REL) {
		printf("This file isn't a relocatable object file\n");
		return 1;
	}

	if (elf_header->section_header_size !=
		sizeof(struct elf64_section_header)) {
		printf("Section header size does not match\n");
		return 1;
	}

	// Setup variable for the section headers
	size_t section_count = elf_header->section_header_count;
	size_t section_header_size = sizeof(struct elf64_section_header);
	struct elf64_section_header *elf_section_headers =
		kmalloc(section_header_size * section_count);
	// Read section headers from the file
	res->read(res, elf_section_headers, elf_header->section_header_offset,
			  section_header_size * section_count);

	printf(
		">> Allocating sections memory and loading string and symbol tables\n");
	for (size_t index = 0; index < section_count; index++) {
		struct elf64_section_header *section = (elf_section_headers + index);

		// If this section should allocate memory and is bigger than 0
		if ((section->flags & ELF_FLAG_ALLOC) && section->size > 0) {
			// Allocate memory, currenty RW so we can write to it
			char *mem = kmalloc(section->size);

			if (section->type == ELF_SECTION_PROGBITS) {
				// Read data from the file
				res->read(res, mem, section->offset, section->size);
			} else if (section->type == ELF_SECTION_NOBITS) {
				// Section is empty, so fill with zeros
				memset(mem, '\0', section->size);
			}
			section->address = (uint64_t)(size_t)mem;
		}

		// Load symbol and string tables from the file
		if (section->type == ELF_SECTION_SYMTAB ||
			section->type == ELF_SECTION_STRTAB) {
			struct elf64_symbol *table = kmalloc(section->size);

			res->read(res, table, section->offset, section->size);

			section->address = (uint64_t)(size_t)table;
		}
	}

	printf(">> Applying symbol relocation\n");
	for (size_t index = 0; index < section_count; index++) {
		struct elf64_section_header *section = (elf_section_headers + index);

		if (section->type == ELF_SECTION_RELA) {
			size_t entry_count = section->size / section->entry_size;
			if (section->entry_size != sizeof(struct elf64_rela_entry)) {
				printf("RELA entry size does not match\n");
				return 1;
			}

			// Load relocation entries from the file
			struct elf64_rela_entry *entries = kmalloc(section->size);
			section->address = (uint64_t)(size_t)entries;
			res->read(res, entries, section->offset, section->size);

			// Locate the section we are relocating
			struct elf64_section_header *relocation_section =
				(elf_section_headers + section->info);
			char *relocation_section_data = (char *)relocation_section->address;

			// Locate the symbol table for this relocation table
			struct elf64_section_header *section_symbol_table =
				(elf_section_headers + section->link);
			struct elf64_symbol *symbol_table =
				(struct elf64_symbol *)(section_symbol_table->address);

			// Locate the string table for the symbol table
			struct elf64_section_header *section_string_table =
				(elf_section_headers + section_symbol_table->link);
			char *string_table = (char *)(section_string_table->address);

			// Relocate all the entries
			for (size_t entry_ind = 0; entry_ind < entry_count; entry_ind++) {
				struct elf64_rela_entry *entry = (entries + entry_ind);

				// Find the symbol for this entry
				struct elf64_symbol *symbol = (symbol_table + entry->symbol);
				char *symbol_name = (string_table + symbol->name_index);

				if (entry->type == ELF_REL_TYPE_64) {
					// Determine the offset in the section
					uint64_t *location =
						(uint64_t *)(relocation_section_data + entry->offset);

					// Check that the symbol is defined in this file
					if (symbol->section > 0) {
						// Print out which symbol is being relocated
						if (symbol->name_index) {
							printf("Relocating symbol: %s\n", symbol_name);
						} else {
							struct elf64_section_header *shstrtab =
								(elf_section_headers +
								 elf_header->string_table_index);
							struct elf64_section_header *section =
								(elf_section_headers + symbol->section);
							printf("Relocating symbol: %s%+lld\n",
								   ((char *)shstrtab->address) +
									   section->name_index,
								   entry->addend);
						}
						// Calculate the location of the symbol
						struct elf64_section_header *symbol_section =
							(elf_section_headers + symbol->section);
						uint64_t symbol_value = symbol_section->address +
												symbol->value + entry->addend;

						// Store the location
						*location = symbol_value;
					} else {
						// The symbol is not defined inside the object file
						// resolve using other rules
						if (strcmp("printf", symbol_name) == 0) {
							*location = (uint64_t)(&printf);
						} else if (strcmp("process_exit", symbol_name) == 0) {
							*location = (uint64_t)(&process_exit);
						} else if (strcmp("thread_exit", symbol_name) == 0) {
							*location = (uint64_t)(&thread_exit);
						} else {
							printf("Unknown symbol: %s\n", symbol_name);
							return 1;
						}
					}
				} else {
					printf("Unknown relocation type: %d\n", entry->type);
					return 1;
				}
			}
		}
	}

	// Dump the sections as a table
	struct elf64_section_header *shstrtab =
		(elf_section_headers + elf_header->string_table_index);
	dump_section_table(elf_section_headers, section_count,
					   (char *)shstrtab->address);

	printf(">> Finding run() function\n");
	void_func_t run_func = 0;
	for (size_t index = 0; index < section_count; index++) {
		struct elf64_section_header *section = (elf_section_headers + index);

		// Look in all symbol tables for the run function
		if (section->type == ELF_SECTION_SYMTAB) {
			struct elf64_symbol *symbol_table =
				(struct elf64_symbol *)section->address;

			// Find the string table for this symbol table
			struct elf64_section_header *section_string_table =
				(elf_section_headers + section->link);
			char *string_table = (char *)(section_string_table->address);

			size_t symbol_count = section->size / section->entry_size;
			for (size_t i = 0; i < symbol_count; i++) {
				// Get the symbol from the table
				struct elf64_symbol *symbol = (symbol_table + i);
				char *symbol_name = (string_table + symbol->name_index);
				struct elf64_section_header *run_section =
					(elf_section_headers + symbol->section);

				// Check if it is the run symbol
				if (strcmp("run", symbol_name) == 0 &&
					((symbol->info >> 4) & 0xF) == 1 &&
					(run_section->flags & ELF_FLAG_EXEC)) {
					// Calculate the symbol location
					run_func =
						(void_func_t)(run_section->address + symbol->value);
					break;
				}
			}
		}

		if (run_func)
			break;
	}

	// Create a process for the run function from the object file
	if (run_func != NULL) {
		printf(">> Running run() function\n");
		process_create("ELF Program", (uintptr_t)run_func, 0, HIGH);
	} else {
		printf("Unable to locate run()\n");
		return 1;
	}

	return 0;
}
