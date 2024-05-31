#ifndef MODULE_H
#define MODULE_H

#include <klibc/vec.h>
#include <stddef.h>
#include <stdint.h>

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

typedef vec_t(struct module *) module_list_t;

extern module_list_t modules_list;

uint64_t module_load(const char *path);
bool module_unload(const char *name);

void module_dump(void);

#endif
