#ifndef GDT_H
#define GDT_H

#include <stddef.h>
#include <stdint.h>

#if !(defined(__x86_64__))
#error "THIS IS ONLY FOR x86_64"
#endif

#define GDT_KERNEL_BASE 0x0
#define GDT_KERNEL_CODE64 0x8
#define GDT_KERNEL_DATA64 0x10
#define GDT_USER_BASE 0x18
#define GDT_USER_DATA64 0x20
#define GDT_USER_CODE64 0x28
#define GDT_TSS 0x30

struct tss {
	uint32_t reserved;
	uint64_t rsp0;
	uint64_t rsp1;
	uint64_t rsp2;
	uint64_t reserved2;
	uint64_t ist1;
	uint64_t ist2;
	uint64_t ist3;
	uint64_t ist4;
	uint64_t ist5;
	uint64_t ist6;
	uint64_t ist7;
	uint64_t reserved3;
	uint16_t reserved4;
	uint16_t iomap_base;
} __attribute__((packed));

#endif
