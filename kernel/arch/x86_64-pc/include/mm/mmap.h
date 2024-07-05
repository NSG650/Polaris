#ifndef MMAP_H
#define MMAP_H

#include <klibc/resource.h>
#include <klibc/vec.h>
#include <mm/vmm.h>

#define PROT_NONE 0x00
#define PROT_READ 0x01
#define PROT_WRITE 0x02
#define PROT_EXEC 0x04

#define MAP_FAILED ((void *)(-1))
#define MAP_FILE 0x00
#define MAP_SHARED 0x01
#define MAP_PRIVATE 0x02
#define MAP_FIXED 0x10
#define MAP_ANON 0x20
#define MAP_ANONYMOUS 0x20
#define MAP_NORESERVE 0x4000

#define MS_ASYNC 0x01
#define MS_INVALIDATE 0x02
#define MS_SYNC 0x04

#define MCL_CURRENT 0x01
#define MCL_FUTURE 0x02

#define POSIX_MADV_NORMAL 0
#define POSIX_MADV_RANDOM 1
#define POSIX_MADV_SEQUENTIAL 2
#define POSIX_MADV_WILLNEED 3
#define POSIX_MADV_DONTNEED 4

#define MADV_NORMAL 0
#define MADV_RANDOM 1
#define MADV_SEQUENTIAL 2
#define MADV_WILLNEED 3
#define MADV_DONTNEED 4
#define MADV_FREE 8

#define MREMAP_MAYMOVE 1
#define MREMAP_FIXED 2

#define MFD_CLOEXEC 1U
#define MFD_ALLOW_SEALING 2U

struct mmap_range_global {
	struct pagemap *shadow_pagemap;
	vec_t(struct mmap_range_local *) locals;
	struct resource *res;
	uintptr_t base;
	size_t length;
	off_t offset;
};

struct mmap_range_local {
	struct pagemap *pagemap;
	struct mmap_range_global *global;
	uintptr_t base;
	size_t length;
	off_t offset;
	int prot;
	int flags;
};

void mmap_list_ranges(struct pagemap *pagemap);
bool mmap_page_in_range(struct mmap_range_global *global, uintptr_t virt,
						uintptr_t phys, int prot);
bool mmap_range(struct pagemap *pagemap, uintptr_t virt, uintptr_t phys,
				size_t length, int prot, int flags);
void *mmap(struct pagemap *pagemap, uintptr_t addr, size_t length, int prot,
		   int flags, struct resource *res, off_t offset);
bool munmap(struct pagemap *pagemap, uintptr_t addr, size_t length);

void syscall_mmap(struct syscall_arguments *args);
void syscall_munmap(struct syscall_arguments *args);
void syscall_mprotect(struct syscall_arguments *args);

#endif
