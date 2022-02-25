#include <fw/madt.h>
#include <io/ports.h>
#include <sys/apic.h>

static void pic_disable(void) {
	outportb(0xA1, 0xFF);
	outportb(0x21, 0xFF);
}

void pic_init(void) {
	// There isn't any PIC
	if (!(madt->flags & 1)) {
		return;
	}
	// Remap the PIC
	outportb(0x20, 0x11);
	outportb(0xA0, 0x11);
	outportb(0x21, 0x20);
	outportb(0xA1, 0x28);
	outportb(0x21, 4);
	outportb(0xA1, 2);
	outportb(0x21, 1);
	outportb(0xA1, 1);
	outportb(0x21, 0);
	outportb(0xA1, 0);

	pic_disable();
}
