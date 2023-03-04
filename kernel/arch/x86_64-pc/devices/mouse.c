#include <debug/debug.h>
#include <devices/mouse.h>
#include <errno.h>
#include <fs/devtmpfs.h>
#include <io/ports.h>
#include <klibc/mem.h>
#include <klibc/resource.h>
#include <sys/apic.h>
#include <sys/isr.h>

// mouse code partly (mostly) taken from
// https://forum.osdev.org/viewtopic.php?t=10247

struct mouse_device *mouse_dev;

static void mouse_wait(int type) {
	int timeout = 100000;

	if (type == 0) {
		while (timeout--) {
			if (inb(0x64) & (1 << 0)) {
				return;
			}
		}
	} else {
		while (timeout--) {
			if (!(inb(0x64) & (1 << 1))) {
				return;
			}
		}
	}
}

static void mouse_write(uint8_t val) {
	mouse_wait(1);
	outb(0x64, 0xd4);
	mouse_wait(1);
	outb(0x60, val);
}

static uint8_t mouse_read(void) {
	mouse_wait(0);
	return inb(0x60);
}

uint8_t mouse_cycle = 0;
struct mouse_packet curr_pack = {0};
struct mouse_packet old_pack = {0};
bool discard_pack = 0;

void mouse_handle(struct regs *reg) {
	switch (mouse_cycle) {
		case 0: {
			curr_pack.flags = inb(0x60);
			mouse_cycle++;
			if (curr_pack.flags & (1 << 6) || curr_pack.flags & (1 << 7))
				discard_pack = 1; // discard rest of packet
			if (!(curr_pack.flags & (1 << 3)))
				discard_pack = 1; // discard rest of packet
			break;
		}

		case 1: {
			curr_pack.x_mov = inb(0x60);
			mouse_cycle++;
			break;
		}

		case 2:
			curr_pack.y_mov = inb(0x60);
			if (discard_pack) {
				discard_pack = 0;
			}
			mouse_cycle = 0;
			/*if (old_pack.x_mov != curr_pack.x_mov && old_pack.y_mov !=
			curr_pack.y_mov && old_pack.flags != curr_pack.flags) { old_pack =
			curr_pack;
			}
			else {
			  discard_pack = 1;
			}*/
			break;
	}
	apic_eoi();
}

int mouse_resource_ioctl(struct resource *this,
						 struct f_description *description, uint64_t request,
						 uint64_t arg) {
	spinlock_acquire_or_wait(this->lock);

	if (discard_pack == 1) {
		spinlock_drop(this->lock);
		return -1;
	}

	struct mouse_device *dev = (struct mouse_device *)this;

	(void)description;

	switch (request) {
		case 0x1:
			spinlock_drop(this->lock);
			memcpy((void *)arg, dev->pack, sizeof(struct mouse_packet));
			memzero(dev->pack, sizeof(struct mouse_packet));
			return 0;
		default:
			spinlock_drop(this->lock);
			return resource_default_ioctl(this, description, request, arg);
	}
	errno = EINVAL;
	spinlock_drop(this->lock);
	return -1;
}

void mouse_init(void) {
	mouse_write(0xf6);
	mouse_read();

	mouse_write(0xf4);
	mouse_read();

	mouse_dev = resource_create(sizeof(struct mouse_device));
	mouse_dev->pack = &curr_pack;

	mouse_dev->res.stat.st_size = 0;
	mouse_dev->res.stat.st_blocks = 0;
	mouse_dev->res.stat.st_blksize = 4096;
	mouse_dev->res.stat.st_rdev = resource_create_dev_id();
	mouse_dev->res.stat.st_mode = 0644 | S_IFCHR;
	mouse_dev->res.ioctl = mouse_resource_ioctl;

	isr_register_handler(60, mouse_handle);
	ioapic_redirect_irq(12, 60);

	devtmpfs_add_device(mouse_dev, "mouse");
}