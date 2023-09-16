#include <stdbool.h>

void kprintffos(bool fos, char *fmt, ...);
#define kprintf(...) kprintffos(1, __VA_ARGS__)

void driver_entry(void) {
	kprintf("Hello kernel module world!\n");
}
