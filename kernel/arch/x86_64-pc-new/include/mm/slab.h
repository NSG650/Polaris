#ifndef SLAB_H
#define SLAB_H

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

#define kcalloc(A, B) kmalloc(A * sizeof(B))

#endif
