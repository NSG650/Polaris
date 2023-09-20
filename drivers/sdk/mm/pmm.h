#ifndef PMM_H
#define PMM_H

#include <stddef.h>
#include <stdint.h>

void *pmm_alloc(size_t pages);
void *pmm_allocz(size_t pages);
void pmm_free(void *addr, size_t pages);

#endif