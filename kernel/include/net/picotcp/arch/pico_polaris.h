#ifndef PICO_SUPPORT_POLARIS
#define PICO_SUPPORT_POLARIS

#include <asm/asm.h>
#include <debug/debug.h>
#include <mm/slab.h>
#include <sys/timer.h>

#define dbg kprintf

#define pico_zalloc(x) kcalloc(x, 1)
#define pico_free(x) kfree(x)

static inline unsigned long PICO_TIME(void) {
	return (unsigned long)(timer_count() / 1000);
}

static inline unsigned long PICO_TIME_MS(void) {
	return (unsigned long)timer_count();
}

static inline void PICO_IDLE(void) {
	pause();
}

#endif
