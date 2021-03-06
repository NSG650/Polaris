#ifndef SYMBOLS_H
#define SYMBOLS_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
	uint64_t address;
	const char *function_name;
} sym_table_t;

const char *symbols_return_function_name(uint64_t address);

#endif
