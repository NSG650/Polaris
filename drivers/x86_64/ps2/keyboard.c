#include "keyboard.h"
#include "ps2.h"
#include <debug/debug.h>
#include <errno.h>
#include <fs/devtmpfs.h>
#include <io/ports.h>
#include <klibc/mem.h>
#include <klibc/time.h>
#include <sched/sched.h>
#include <sys/apic.h>
#include <sys/timer.h>

struct resource *keyboard_resource = NULL;
bool new_input = false;

static ssize_t keyboard_dev_read(struct resource *this,
								 struct f_description *description, void *buf,
								 off_t offset, size_t count) {
	(void)this;
	(void)offset;
	if (count < 2) {
		errno = EINVAL;
		return -1;
	}

	spinlock_acquire_or_wait(&this->lock);

	ssize_t ret = 0;
	if (!new_input) {
		spinlock_drop(&this->lock);
		if (description->flags & O_NONBLOCK) {
			errno = EAGAIN;
			return -1;
		}
		struct event *events[] = {&this->event};
		event_await(events, 1, true);
		spinlock_acquire_or_wait(&this->lock);
	}
	new_input = false;

	uint8_t data[2] = {0};
	data[0] = ps2_read();
	ret = 1;
	if (data[0] == 0xe0) {
		if (!new_input) {
			spinlock_drop(&this->lock);
			if (description->flags & O_NONBLOCK) {
				errno = EAGAIN;
				return -1;
			}
			struct event *events[] = {&this->event};
			event_await(events, 1, true);
			spinlock_acquire_or_wait(&this->lock);
		}

		new_input = false;
		data[1] = ps2_read();
		ret = 2;
	}

	memcpy(buf, data, ret);
	this->status &= ~POLLIN;
	spinlock_drop(&this->lock);
	return ret;
}

static void keyboard_interrupt_handle(registers_t *r) {
	cli();
	new_input = true;
	keyboard_resource->status |= POLLIN;
	event_trigger(&keyboard_resource->event, false);
	sti();
	apic_eoi();
}

void keyboard_init(void) {
	keyboard_resource = resource_create(sizeof(struct resource));

	keyboard_resource->stat.st_size = 0;
	keyboard_resource->stat.st_blocks = 0;
	keyboard_resource->stat.st_blksize = 512;
	keyboard_resource->stat.st_rdev = resource_create_dev_id();
	keyboard_resource->stat.st_mode = 0644 | S_IFCHR;
	keyboard_resource->read = keyboard_dev_read;
	keyboard_resource->status |= POLLOUT;

	devtmpfs_add_device(keyboard_resource, "keyboard");

	isr_register_handler(49, keyboard_interrupt_handle);
	ioapic_redirect_irq(1, 49);
}
