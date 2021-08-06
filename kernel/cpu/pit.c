#include "pit.h"
#include "ports.h"

volatile uint64_t global_tick = 0;

void timer_handler(registers_t *r) {
	(void)r;
	global_tick++;
}

void set_pit_freq(uint64_t freq) {
	uint16_t divisor = 1193180 / freq;

	uint8_t low = (uint8_t)(divisor & 0xFF);
	uint8_t high = (uint8_t)((divisor >> 8) & 0xFF);

	port_byte_out(0x43, 0x34);
	port_byte_out(0x40, low);
	port_byte_out(0x40, high);
}

void wait(uint64_t ticks) {
	volatile uint64_t end = global_tick + ticks;
	while (global_tick < end)
		;
}
