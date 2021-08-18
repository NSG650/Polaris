#include "idt.h"

idt_gate_t idt[IDT_ENTRIES];
idt_register_t idt_reg;

void set_idt_gate(int n, uint64_t handler) {
	idt[n].low_offset = (uint16_t)(handler >> 0);
	idt[n].sel = KERNEL_CS;
	idt[n].always0 = 0;
	idt[n].flags = 0x8E;
	idt[n].mid_offset = (uint16_t)(handler >> 16);
	idt[n].high_offset = (uint32_t)(handler >> 32);
	idt[n].always0again = 0;
}

void set_idt(void) {
	idt_reg.base = (uint64_t)&idt;
	idt_reg.limit = IDT_ENTRIES * sizeof(idt_gate_t) - 1;
	asm volatile("lidtq %0" : "=m"(idt_reg));
}
