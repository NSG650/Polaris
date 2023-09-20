#ifndef ASM_H
#define ASM_H

#include <stdint.h>

#define FLAT_PTR(PTR) (*((uintptr_t(*)[])(PTR)))

#define BYTE_PTR(PTR) (*((uint8_t *)(PTR)))
#define WORD_PTR(PTR) (*((uint16_t *)(PTR)))
#define DWORD_PTR(PTR) (*((uint32_t *)(PTR)))
#define QWORD_PTR(PTR) (*((uint64_t *)(PTR)))

void halt(void);
void cli(void);
void sti(void);
void pause(void);
void nop(void);

#endif
