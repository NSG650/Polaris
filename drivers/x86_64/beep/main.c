#include <debug/debug.h>
#include <io/ports.h>
#include <klibc/mem.h>
#include <klibc/module.h>

static void beep_play_sound(uint32_t freq) {
	uint32_t div = 0;
	uint8_t tmp = 0;

	// Set the PIT to the desired frequency
	div = 1193180 / freq;
	outb(0x43, 0xb6);
	outb(0x42, (uint8_t)(div));
	outb(0x42, (uint8_t)(div >> 8));

	// And play the sound using the PC speaker
	tmp = inb(0x61);
	if (tmp != (tmp | 3)) {
		outb(0x61, tmp | 3);
	}
}

static void beep_no_sound(void) {
	uint8_t tmp = inb(0x61) & 0xFC;

	outb(0x61, tmp);
}

void timer_sleep(uint64_t ms);

void driver_exit(void) {
	kprintf("Bye bye!\n");
}

uint64_t driver_entry(struct module *driver_module) {
	driver_module->exit = driver_exit;

	kprintf("Hello I am the beep driver!\n");

	beep_play_sound(220);
	timer_sleep(1000);
	beep_no_sound();

	return 0;
}
