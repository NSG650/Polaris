#include <debug/debug.h>
#include <io/mmio.h>
#include <io/ports.h>
#include <klibc/time.h>
#include <mm/vmm.h>
#include <sys/apic.h>
#include <sys/hpet.h>
#include <sys/isr.h>
#include <sys/pit.h>
#include <sys/prcb.h>
#include <sys/timer.h>

static struct hpet_table *hpet_table = NULL;
static struct hpet *hpet = NULL;
static uint32_t clk = 0;
static bool timer_installed_b = false;

static volatile struct limine_boot_time_request boot_time_request = {
	.id = LIMINE_BOOT_TIME_REQUEST, .revision = 0};

bool hpet_init(void) {
	hpet_table = acpi_find_sdt("HPET", 0);
	if (!hpet_table) {
		kprintf("HPET: No HPET found!\n");
		return false;
	}
	hpet = (struct hpet *)(hpet_table->address.base + MEM_PHYS_OFFSET);

	kprintf("HPET: HPET at %p\n", (void *)hpet);

	clk = hpet->general_capabilities >> 32;

	/*
	 * General Configuration Register
	 *     0 - main counter is halted, timer interrupts are disabled
	 *     1 - main counter is running, timer interrupts are allowed if enabled
	 */
	mmoutq(&hpet->general_configuration, 0);
	mmoutq(&hpet->main_counter_value, 0);
	mmoutq(&hpet->general_configuration, 1);
	timer_installed_b = true;
	return true;
}

uint64_t hpet_counter_value(void) {
	return mminq(&hpet->main_counter_value);
}

void hpet_sleep(uint64_t us) {
	uint64_t target = hpet_counter_value() + (us * 1000000000) / clk;
	while (hpet_counter_value() < target)
		pause();
}

void pit_set_reload_value(uint16_t new_count) {
	outb(0x43, 0x34);
	outb(0x40, (uint8_t)new_count);
	outb(0x40, (uint8_t)(new_count >> 8));
}

void pit_set_frequency(uint64_t frequency) {
	uint64_t new_divisor = PIT_DIVIDEND / frequency;
	if (PIT_DIVIDEND % frequency > frequency / 2) {
		new_divisor++;
	}
	pit_set_reload_value((uint16_t)new_divisor);
}

void pit_init(void) {
	timer_installed_b = true;
	pit_set_reload_value(0xffff);
	pit_set_frequency(TIMER_FREQ);
}

uint16_t pit_counter_value(void) {
	outb(0x43, 0x00);
	uint8_t lo = inb(0x40);
	uint8_t hi = inb(0x40);
	return ((uint16_t)hi << 8) | lo;
}

void pit_sleep(uint64_t ms) {
	uint64_t target = pit_counter_value() + ms;
	while (pit_counter_value() < target)
		pause();
}

void timer_init(void) {
	struct limine_boot_time_response *boot_time_resp =
		boot_time_request.response;

	time_realtime.tv_sec = boot_time_resp->boot_time;

	if (hpet_init()) {
		return;
	}
	pit_init();
}

bool timer_installed(void) {
	return timer_installed_b;
}

void timer_sleep(uint64_t ms) {
	if (hpet_table) {
		return hpet_sleep(ms * 1000);
	}
	pit_sleep(ms);
}

uint64_t timer_count(void) {
	if (hpet_table) {
		return (hpet_counter_value() * clk) / (1000000000000);
	}
	return pit_counter_value();
}

void time_init(void) {
	syscall_register_handler(0x13a, syscall_getclock);
	syscall_register_handler(0x23, syscall_nanosleep);
}