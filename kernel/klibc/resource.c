#include <stddef.h>
#include "resource.h"
#include "alloc.h"
#include "types.h"

// These functions should be stubs for generic kernel handles unused functions.

static int stub_close(struct resource *this) {
    (void)this;
    return -1;
}

static ssize_t stub_read(struct resource *this, void *buf, off_t loc, size_t count) {
    (void)this;
    (void)buf;
    (void)loc;
    (void)count;
    return -1;
}

static ssize_t stub_write(struct resource *this, const void *buf, off_t loc, size_t count) {
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
    struct resource *new = alloc(actual_size);

    new->actual_size = actual_size;

    new->close = stub_close;
    new->read  = stub_read;
    new->write = stub_write;
    new->ioctl = stub_ioctl;

    return new;
}
