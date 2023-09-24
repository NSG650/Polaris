#include "debug/debug.h"
#include <errno.h>
#include <fb/fb.h>
#include <klibc/mem.h>
#include <mm/mmap.h>

struct fbdev_info {
	size_t pitch, bpp;
	uint16_t width, height;
};

static struct fbdev_info info;
static struct resource *framebuff_res;

static ssize_t fbdev_write(struct resource *this,
						   struct f_description *description, const void *buf,
						   off_t offset, size_t count) {
	(void)description;
	spinlock_acquire_or_wait(&this->lock);
	memcpy((void *)(framebuff.address + offset), buf, count);
	spinlock_drop(&this->lock);
	return 0;
}

static void *fbdev_mmap(struct resource *this, size_t file_page, int flags) {
	(void)flags;
	spinlock_acquire_or_wait(&this->lock);
	size_t offset = file_page * PAGE_SIZE;

	if (offset >= (framebuff.height * framebuff.pitch)) {
		return NULL;
	}

	spinlock_drop(&this->lock);

	return (void *)(((uint64_t)(framebuff.address) - MEM_PHYS_OFFSET) + offset);
}

static int fbdev_ioctl(struct resource *this, struct f_description *description,
					   uint64_t request, uint64_t arg) {
	spinlock_acquire_or_wait(&this->lock);
	switch (request) {
		case 0x1:
			memcpy((void *)arg, &info, sizeof(struct fbdev_info));
			spinlock_drop(&this->lock);
			return 0;
		default:
			spinlock_drop(&this->lock);
			return resource_default_ioctl(this, description, request, arg);
	}
	spinlock_drop(&this->lock);
	errno = EINVAL;
	return -1;
}

void fbdev_init(void) {
	extern uint8_t framebuffer_initialised;
	if (!framebuffer_initialised)
		return;
	info.height = framebuff.height;
	info.width = framebuff.width;
	info.pitch = framebuff.pitch;
	info.bpp = framebuff.bpp;

	framebuff_res = resource_create(sizeof(struct resource));

	framebuff_res->ioctl = fbdev_ioctl;
	framebuff_res->write = fbdev_write;
	framebuff_res->mmap = fbdev_mmap;
	framebuff_res->can_mmap = true;

	devtmpfs_add_device(framebuff_res, "fbdev");
}
