#include <fw/madt.h>
#include <io/ports.h>
#include <sys/apic.h>

static void pic_disable(void) {
	outb(0xA1, 0xFF);
	outb(0x21, 0xFF);
}

void pic_init(void) {
	// There isn't any PIC
	if (!(madt->flags & 1)) {
		return;
	}
	// Remap the PIC
	outb(0x20, 0x11);
	outb(0xA0, 0x11);
	outb(0x21, 0x20);
	outb(0xA1, 0x28);
	outb(0x21, 4);
	outb(0xA1, 2);
	outb(0x21, 1);
	outb(0xA1, 1);
	outb(0x21, 0);
	outb(0xA1, 0);

	pic_disable();
}
