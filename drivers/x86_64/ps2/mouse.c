#include "mouse.h"
#include "ps2.h"
#include <debug/debug.h>
#include <errno.h>
#include <fs/devtmpfs.h>
#include <io/ports.h>
#include <klibc/mem.h>
#include <sched/sched.h>
#include <sys/apic.h>
#include <sys/timer.h>

void *memcpy(void *dest, const void *src, size_t n) {
	uint8_t *pdest = (uint8_t *)dest;
	const uint8_t *psrc = (const uint8_t *)src;

	for (size_t i = 0; i < n; i++) {
		pdest[i] = psrc[i];
	}

	return dest;
}

void *memset(void *b, int c, size_t len) {
	size_t i = 0;
	unsigned char *p = b;
	while (len > 0) {
		*p = c;
		p++;
		len--;
	}
	return b;
}

static inline void mouse_wait(int type) {
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

static inline void mouse_write(uint8_t val) {
	mouse_wait(1);
	outb(0x64, 0xd4);
	mouse_wait(1);
	outb(0x60, val);
}

static inline uint8_t mouse_read(void) {
	mouse_wait(0);
	return inb(0x60);
}

struct mouse_dev {
	struct resource res;
	bool new_packet;
	struct mouse_packet packet;
};

static struct mouse_dev *mouse_resource = NULL;
static struct event mouse_interrupt_event = {0};

static void mouse_interrupt_handle(registers_t *r) {
	cli();
	event_trigger(&mouse_interrupt_event, false);
	apic_eoi();
	sti();
}

static _Noreturn void mouse_thread(void) {
	// Apparently during startup there is some garbage mouse data usually
	// 250-300 ms from boot and trips the mouse cycle. Ignore those.
	uint64_t sleep_till = timer_get_sleep_ns(300 * 1000);
	while (timer_get_abs_count() <= sleep_till) {
		inb(0x60);
	}

	uint8_t mouse_cycle = 0;
	struct mouse_packet packet = {0};
	bool discard_packet = false;

	for (;;) {
		struct event *events[] = {&mouse_interrupt_event};
		event_await(events, 1, true);

		switch (mouse_cycle) {
			case 0: {
				packet.flags = inb(0x60);
				mouse_cycle++;
				if (packet.flags & (1 << 6) || packet.flags & (1 << 7))
					discard_packet = true; // discard rest of packet
				if (!(packet.flags & (1 << 3)))
					discard_packet = true; // discard rest of packet

				continue;
			}

			case 1: {
				packet.delta_x = mouse_read();
				mouse_cycle++;
				continue;
			}

			case 2: {
				packet.delta_y = mouse_read();
				mouse_cycle = 0;

				if (discard_packet) {
					discard_packet = !discard_packet;
					continue;
				}
				break;
			}
		}

		if (packet.flags & (1 << 4)) {
			packet.delta_x = (int8_t)(uint8_t)packet.delta_x;
		}

		if (packet.flags & (1 << 5)) {
			packet.delta_y = (int8_t)(uint8_t)packet.delta_y;
		}

		spinlock_acquire_or_wait(&mouse_resource->res.lock);
		memcpy(&mouse_resource->packet, &packet, sizeof(struct mouse_packet));
		mouse_resource->new_packet = true;
		spinlock_drop(&mouse_resource->res.lock);

		//       mouse_resource->res.status |= POLLIN;

		event_trigger(&mouse_resource->res.event, false);
	}
}

static ssize_t mouse_dev_read(struct resource *this,
							  struct f_description *description, void *buf,
							  off_t offset, size_t count) {
	(void)this;
	(void)offset;

	if (count != sizeof(struct mouse_packet)) {
		errno = EINVAL;
		return -1;
	}

	if (description->flags & O_NONBLOCK) {
		errno = EAGAIN;
		return -1;
	}

	while (!mouse_resource->new_packet) {
		struct event *events[] = {&mouse_resource->res.event};
		event_await(events, 1, true);
	}

	spinlock_acquire_or_wait(&mouse_resource->res.lock);

	memcpy(buf, &mouse_resource->packet, sizeof(struct mouse_packet));
	mouse_resource->new_packet = false;

	// mouse_res->status &= ~POLLIN;

	spinlock_drop(&mouse_resource->res.lock);

	return sizeof(struct mouse_packet);
}

void mouse_init(void) {
	mouse_write(0xf6);
	mouse_read();

	mouse_write(0xf4);
	mouse_read();

	mouse_resource = resource_create(sizeof(struct mouse_dev));

	mouse_resource->res.stat.st_size = 0;
	mouse_resource->res.stat.st_blocks = 0;
	mouse_resource->res.stat.st_blksize = 512;
	mouse_resource->res.stat.st_rdev = resource_create_dev_id();
	mouse_resource->res.stat.st_mode = 0644 | S_IFCHR;
	mouse_resource->res.read = mouse_dev_read;

	devtmpfs_add_device((struct resource *)mouse_resource, "mouse");

	isr_register_handler(60, mouse_interrupt_handle);
	ioapic_redirect_irq(12, 60);

	// the first process is always the kernel process
	thread_create((uintptr_t)mouse_thread, 0, false, process_list);
}