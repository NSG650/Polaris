#ifndef SLAB_H
#define SLAB_H

#include <klibc/mem.h>
#include <stddef.h>

void slab_init(void);
void *slab_alloc(size_t size);
void *slab_realloc(void *addr, size_t size);
void slab_free(void *addr);

static inline void *kmalloc(size_t size) {
	return slab_alloc(size);
}

static inline void *krealloc(void *addr, size_t size) {
	return slab_realloc(addr, size);
}

static inline void kfree(void *addr) {
	return slab_free(addr);
}

static inline void *kcalloc(size_t count, size_t size) {
	void *ret = kmalloc(count * size);
	if (ret == NULL) {
		return NULL;
	}
	memzero(ret, count * size);
	return ret;
}

#endif
