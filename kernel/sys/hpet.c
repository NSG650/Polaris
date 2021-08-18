#include <stddef.h>
#include <stdint.h>

#include "../acpi/acpi.h"
#include "../kernel/panic.h"
#include "../klibc/printf.h"
#include "../mm/vmm.h"
#include "hpet.h"
#include "mmio.h"

struct address_structure {
	uint8_t address_space_id; // 0 - system memory, 1 - system I/O
	uint8_t register_bit_width;
	uint8_t register_bit_offset;
	uint8_t reserved;
	uint64_t address;
} __attribute__((packed));

struct HpetTable {
	struct sdt header;
	uint8_t hardware_rev_id;
	uint8_t comparator_count : 5;
	uint8_t counter_size : 1;
	uint8_t reserved : 1;
	uint8_t legacy_replacement : 1;
	uint16_t pci_vendor_id;
	struct address_structure address;
	uint8_t hpet_number;
	uint16_t minimum_tick;
	uint8_t page_protection;
} __attribute__((packed));

struct Hpet {
	uint64_t general_capabilities;
	uint64_t unused0;
	uint64_t general_configuration;
	uint64_t unused1;
	uint64_t general_int_status;
	uint64_t unused2;
	uint64_t unused3[24];
	uint64_t main_counter_value;
	uint64_t unused4;
};

static struct HpetTable *hpet_table;
static struct Hpet *hpet;
static uint32_t clk = 0;

void hpet_init(void) {
	hpet_table = acpi_find_sdt("HPET");
	if (!hpet_table) {
		PANIC("D requires a HPET to be installed");
	}
	hpet = (struct Hpet *)(hpet_table->address.address + MEM_PHYS_OFFSET);

	clk = hpet->general_capabilities >> 32;

	mmoutq(&hpet->general_configuration, 0);
	mmoutq(&hpet->main_counter_value, 0);
	mmoutq(&hpet->general_configuration, 1);

	hpet_usleep(1);
}

void hpet_usleep(uint64_t us) {
	uint64_t target =
	  mminq(&hpet->main_counter_value) + (us * 1000000000) / clk;
	while (mminq(&hpet->main_counter_value) < target) {
		printf("&hpet->main_counter_value: %lld Target Value: %lld\n",
			   mminq(&hpet->main_counter_value), target);
	}
}
