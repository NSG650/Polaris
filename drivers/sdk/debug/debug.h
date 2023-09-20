#ifndef DEBUG_H
#define DEBUG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void kprintffos(bool fos, char *fmt, ...);
void panic_(size_t *ip, size_t *bp, char *fmt, ...);

#define panic(...) \
	panic_(__builtin_return_address(0), __builtin_frame_address(0), __VA_ARGS__)

#define kprintf(...) kprintffos(0, __VA_ARGS__)

// printf debugging best debugging
#define crash_or_not() kprintf("Do we crash? %s:%d\n", __FILE__, __LINE__)

#endif