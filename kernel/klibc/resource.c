#include "resource.h"
#include "types.h"
#include <liballoc.h>
#include <stddef.h>

static int stub_close(struct resource *this) {
	(void)this;
	return -1;
}

static ssize_t stub_read(struct resource *this, void *buf, off_t loc,
						 size_t count) {
	(void)this;
	(void)buf;
	(void)loc;
	(void)count;
	return -1;
}

static ssize_t stub_write(struct resource *this, const void *buf, off_t loc,
						  size_t count) {
	(void)this;
	(void)buf;
	(void)loc;
	(void)count;
	return -1;
}

static int stub_ioctl(struct resource *this, int request, ...) {
	(void)this;
	(void)request;
	return -1;
}

void *resource_create(size_t actual_size) {
	struct resource *new = kmalloc(actual_size);

	new->actual_size = actual_size;

	new->close = stub_close;
	new->read = stub_read;
	new->write = stub_write;
	new->ioctl = stub_ioctl;

	return new;
}
