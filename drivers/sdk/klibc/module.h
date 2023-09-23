#ifndef MODULE_H
#define MODULE_H

#include <stddef.h>
#include <stdint.h>

#include <klibc/vec.h>

struct module;

struct mapping {
	uintptr_t addr;
	size_t size;
	uint64_t prot;
};

typedef uint64_t (*module_entry_t)(struct module *);
typedef void (*void_func_t)(void);

struct module {
	char name[128];
	module_entry_t entry_point;
	void_func_t exit;
	size_t mappings_count;
	struct mapping mappings[16];
};

uint64_t module_load(const uint8_t *data);
bool module_unload(const char *name);

#endif