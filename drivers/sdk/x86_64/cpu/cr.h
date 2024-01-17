#ifndef CR_H
#define CR_H

#define write_cr(reg, val) \
	asm volatile("mov cr" reg ", %0" ::"r"(val) : "memory");

#define read_cr(reg)                                         \
	({                                                       \
		size_t cr;                                           \
		asm volatile("mov %0, cr" reg : "=r"(cr)::"memory"); \
		cr;                                                  \
	})

#endif
