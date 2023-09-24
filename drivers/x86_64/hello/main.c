#include <debug/debug.h>
#include <klibc/mem.h>
#include <klibc/module.h>

void driver_exit(void) {
	kprintf("Bye bye!\n");
}

uint64_t driver_entry(struct module *driver_module) {
	driver_module->exit = driver_exit;

	kprintf("Hello kernel module world!\n");

	return 0;
}
