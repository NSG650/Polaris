#include <asm/asm.h>

void halt(void) {
	asm("hlt");
}

void cli(void) {
	asm("cli");
}

void sti(void) {
	asm("sti");
}

void pause(void) {
	asm("pause");
}

void nop(void) {
	asm("nop");
}
