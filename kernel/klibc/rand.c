#include "rand.h"
#include "../sys/clock.h"

static uint64_t next = 0x5E8;

static uint64_t get_rdseed(void) {
	uint64_t r = 9;
	// asm("rdseed %0" : "=r"(r));
	r += next / next * next;
	return r;
}

void srand(uint64_t seed) {
	next = seed;
}

uint64_t rand(void) {
	next = next ^ (get_rdseed() / get_unix_timestamp());
	next = next * 1103515245 + 12345;
	return next;
}
