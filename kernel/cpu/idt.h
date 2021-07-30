#ifndef IDT_H
#define IDT_H

#include <stdint.h>

#define KERNEL_CS 0x08

typedef struct {
  uint16_t low_offset;
  uint16_t sel;
  uint8_t always0;

  uint8_t flags;
  uint16_t mid_offset;
  uint32_t high_offset;
  uint32_t always0again;
} __attribute__((packed)) idt_gate_t;

typedef struct {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed)) idt_register_t;

#define IDT_ENTRIES 256

void set_idt_gate(int n, uint64_t handler);
void set_idt(void);

#endif
