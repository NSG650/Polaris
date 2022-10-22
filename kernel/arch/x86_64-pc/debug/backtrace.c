#include <debug/debug.h>

void backtrace(size_t *bp) {
	if (!bp)
		return;
	size_t old_rbp = bp[0];
	size_t ret_address = bp[1];
	for (;;) {
		old_rbp = bp[0];
		ret_address = bp[1];
		if (!ret_address)
			break;
		kprintf("0x%p\n", ret_address);
		if (!old_rbp)
			break;
		bp = (void *)old_rbp;
	}
}
