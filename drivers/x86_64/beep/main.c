#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) {
	asm volatile("out %0, %1\n\t" : : "d"(port), "a"(val) : "memory");
}

static inline uint8_t inb(uint16_t port) {
	uint8_t ret;
	asm volatile("in %0, %1\n\t" : "=a"(ret) : "d"(port) : "memory");
	return ret;
}

void kprintffos(bool fos, char *fmt, ...);
#define kprintf(...) kprintffos(1, __VA_ARGS__)

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

void driver_entry(void) {
	kprintf("Hello I am the beep driver!\n");
	beep_play_sound(220);
	timer_sleep(1000);
	beep_no_sound();
}