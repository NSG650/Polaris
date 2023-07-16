#ifndef SLAB_H
#define SLAB_H

#include <stddef.h>
#include <stdint.h>

static void *kcalloc(size_t size, size_t size_of_datatype) {
	(void)size;
	(void)size_of_datatype;
	return NULL;
}

static void *kmalloc(size_t size) {
	(void)size;
	return NULL;
}

static void kfree(void *ptr) {
	(void)ptr;
	return;
}

#endif
