#include <debug/debug.h>
#include <fb/fb.h>
#include <fs/devtmpfs.h>
#include <klibc/mem.h>
#include <klibc/module.h>
#include <x86_64/io/ports.h>

uint64_t driver_entry(struct module *driver_module) {
	return 0;
}
