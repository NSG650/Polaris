#include <stdint.h>
#include <stddef.h>
#include "../video/video.h"
#include "../cpu/idt.h"
#include "../cpu/isr.h"
#include "../acpi/acpi.h"
#include "../klibc/printf.h"
#include "../cpu/pit.h"
#include "../mm/pmm.h"
#include "../mm/vmm.h"
#include "../sys/hpet.h"
#include "../serial/serial.h"
#include <stivale2.h>
#include "panic.h"
#include "../cpu/ports.h"
#include "../sys/clock.h"

extern void init_gdt(void);

static uint8_t stack[4096];
static struct stivale2_header_tag_framebuffer framebuffer_hdr_tag = {
    // All tags need to begin with an identifier and a pointer to the next tag.
    .tag = {
        .identifier = STIVALE2_HEADER_TAG_FRAMEBUFFER_ID,
        .next = 0
    },
    .framebuffer_width  = 0,
    .framebuffer_height = 0,
    .framebuffer_bpp    = 0
};

__attribute__((section(".stivale2hdr"), used))
static struct stivale2_header stivale_hdr = {
    .entry_point = 0,
    .stack = (uintptr_t)stack + sizeof(stack),
    .flags = 0,
    .tags = (uintptr_t)&framebuffer_hdr_tag
};

void *stivale2_get_tag(struct stivale2_struct *stivale2_struct, uint64_t id) {
    struct stivale2_tag *current_tag = (void *)stivale2_struct->tags;
    for (;;) {

        if (current_tag == NULL) {
            return NULL;
        }

        if (current_tag->identifier == id) {
            return current_tag;
        }

        current_tag = (void *)current_tag->next;
    }
}

void _start(struct stivale2_struct *stivale2_struct) {
    stivale2_struct = (void *)stivale2_struct + MEM_PHYS_OFFSET;
    init_gdt();
    struct stivale2_struct_tag_framebuffer *fb_str_tag;
    fb_str_tag = stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_FRAMEBUFFER_ID);
    if (fb_str_tag == NULL) {
       write_serial("FAILED TO AQUIRE A FRAMEBUFFER\n\nHALTING");
       for (;;)
            __asm__("hlt");
    }
    video_init(fb_str_tag);
    struct stivale2_struct_tag_memmap *memmap_tag = stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_MEMMAP_ID);
    pmm_init((void*)memmap_tag->memmap, memmap_tag->entries);
    vmm_init((void*)memmap_tag->memmap, memmap_tag->entries);
    struct stivale2_struct_tag_rsdp *rsdp_tag = stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_RSDP_ID);
    serial_install();
    isr_install();
    __asm__ volatile("sti");
    //set_pit_freq(100);
    acpi_init((void *)rsdp_tag->rsdp + MEM_PHYS_OFFSET);
    hpet_init();
    printf("Hello World!\n");
    printf("%d\n", get_unix_timestamp());
    for (;;)
        __asm__("hlt");
}
