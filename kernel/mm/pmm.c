#include "pmm.h"
#include "../kernel/die.h"
#include "../klibc/bitman.h"
#include "../klibc/mem.h"
#include "../klibc/math.h"
#include <stivale2.h>

static void *bitmap;
static size_t last_used_index = 0;
static uintptr_t highest_page = 0;


void pmm_init(struct stivale2_mmap_entry* memap, size_t memap_entries) {
    for(size_t i = 0; i < memap_entries; i++) {
        if(memap[i].type != STIVALE2_MMAP_USABLE)
            continue;
        uintptr_t top = memap[i].base + memap[i].length;
        if(top > highest_page)
            highest_page = top;
    }

    size_t bitmap_size = DIV_ROUNDUP(highest_page, (size_t)0x1000) / 8;

    for(size_t i = 0; i < memap_entries; i++) {
        if(memap[i].type != STIVALE2_MMAP_USABLE)
            continue;
        if (memap[i].length >= bitmap_size) {
            bitmap = (void*)(memap[i].base + (uintptr_t)0xffff800000000000);
            memset(bitmap, 0xff, bitmap_size);
            memap[i].length -= bitmap_size;
            memap[i].base += bitmap_size;
            break;
        }
    }
    for(size_t i = 0; i < memap_entries; i++) {
        if(memap[i].type != STIVALE2_MMAP_USABLE)
            continue;
        for(uintptr_t j = 0; j < memap[i].length; j += 0x1000)
            bitmap_unset(bitmap, (memap[i].base + j)/0x1000);
    }
}

static void *inner_alloc(size_t count, size_t limit) {
    size_t p = 0;

    while (last_used_index < limit) {
        if (!bitmap_test(bitmap, last_used_index++)) {
            if (++p == count) {
                size_t page = last_used_index - count;
                for (size_t i = page; i < last_used_index; i++) {
                    bitmap_set(bitmap, i);
                }
                return (void *)(page * 0x1000);
            }
        } else {
            p = 0;
        }
    }
    return NULL;
}

void *pmm_alloc(size_t count) {
    size_t l = last_used_index;
    void *ret = inner_alloc(count, highest_page / 0x1000);
    if (ret == NULL) {
        last_used_index = 0;
        ret = inner_alloc(count, l);
    }
    return ret;
}

void *pmm_allocz(size_t count) {
    char *ret = (char *)pmm_alloc(count);
    if (ret == NULL)
        return NULL;
    uint64_t *ptr = (uint64_t *)(ret + (uintptr_t)0xffff800000000000);
    for (size_t i = 0; i < count * (0x1000 / sizeof(uint64_t)); i++)
        ptr[i] = 0;
    return ret;
}

void pmm_free(void *ptr, size_t count) {
    size_t page = (size_t)ptr / 0x1000;
    for (size_t i = page; i < page + count; i++)
        bitmap_unset(bitmap, i);
}

uintptr_t return_highest_page() {
    return highest_page;
}
