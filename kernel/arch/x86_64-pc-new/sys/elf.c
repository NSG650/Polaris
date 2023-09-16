#include <debug/debug.h>
#include <klibc/hashmap.h>
#include <mm/vmm.h>
#include <sys/elf.h>

static struct function_symbol *name_to_function = NULL;
static struct function_symbol *function_to_name = NULL;
static size_t function_table_size = 0;
bool symbol_table_initialised = false;

static void simple_append_name(const char *string, uint64_t address) {
	size_t index = hash(string, strlen(string)) % function_table_size;
	name_to_function[index].address = address;
	name_to_function[index].name = string;
}

char *elf_get_name_from_function(uint64_t address) {
	if (!symbol_table_initialised)
		return "";

	address -= KERNEL_BASE;
	address += 0xffffffff80000000;

	// I HATE HASH COLLISIONS
	// I HATE HASH COLLISIONS
	for (size_t i = 0; i < function_table_size; i++) {
		if (function_to_name[i].address == address)
			return function_to_name[i].name;
	}

	return "UNKNOWN";
}

uint64_t elf_get_function_from_name(const char *string) {
	if (!symbol_table_initialised)
		return NULL;

	size_t index = hash(string, strlen(string)) % function_table_size;
	uint64_t address = name_to_function[index].address;

	address -= 0xffffffff80000000;
	address += KERNEL_BASE;

	return address;
}

void elf_init_function_table(uint8_t *binary) {
	Elf64_Ehdr *ehdr = (Elf64_Ehdr *)binary;

	Elf64_Shdr *sh_table = (Elf64_Shdr *)(binary + ehdr->e_shoff);

	Elf64_Shdr *symtab = NULL;
	char *strtab = NULL;

	for (int i = 0; i < ehdr->e_shnum; i++) {
		if (sh_table[i].sh_type == SHT_SYMTAB) {
			symtab = &sh_table[i];
			strtab = binary + sh_table[symtab->sh_link].sh_offset;
			break;
		}
	}

	if (symtab != NULL) {
		Elf64_Sym *sym_entries = (Elf64_Sym *)(binary + symtab->sh_offset);
		size_t num_symbols = symtab->sh_size / symtab->sh_entsize;

		function_table_size = num_symbols;
		name_to_function =
			kmalloc(sizeof(struct function_symbol) * num_symbols);
		function_to_name =
			kmalloc(sizeof(struct function_symbol) * num_symbols);

		for (size_t i = 0; i < num_symbols; i++) {
			if (ELF64_ST_TYPE(sym_entries[i].st_info) == STT_FUNC &&
				ELF64_ST_BIND(sym_entries[i].st_info) == STB_GLOBAL) {
				char *dupped =
					strdup((char *)((uint64_t)strtab + sym_entries[i].st_name));

				// We need a better hash function for the addresses since there
				// are hash collisions. For now we are using this slow way.
				function_to_name[i].address = sym_entries[i].st_value;
				function_to_name[i].name = dupped;

				simple_append_name(dupped, sym_entries[i].st_value);
			}
		}
		symbol_table_initialised = true;
	}
}