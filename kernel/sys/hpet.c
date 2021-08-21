/*
 * Copyright 2021 Misha
 * Copyright 2021 NSG650
 * Copyright 2021 Sebastian
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hpet.h"
#include "../acpi/acpi.h"
#include "../acpi/fadt.h"
#include "../kernel/panic.h"
#include "../klibc/printf.h"
#include "../mm/vmm.h"
#include "mmio.h"
#include <stddef.h>

struct HpetTable {
	struct sdt header;
	uint8_t hardware_rev_id;
	uint8_t comparator_count : 5;
	uint8_t counter_size : 1;
	uint8_t reserved : 1;
	uint8_t legacy_replacement : 1;
	uint16_t pci_vendor_id;
	struct GenericAddressStructure address;
	uint8_t hpet_number;
	uint16_t minimum_tick;
	uint8_t page_protection;
} __attribute__((packed));

struct Hpet {
	uint64_t general_capabilities;
	uint64_t reserved;
	uint64_t general_configuration;
	uint64_t reserved2;
	uint64_t general_int_status;
	uint64_t reserved3;
	uint64_t reserved4[24];
	uint64_t main_counter_value;
	uint64_t reserved5;
};

static struct HpetTable *hpet_table;
static struct Hpet *hpet;
static uint32_t clk = 0;

void hpet_init(void) {
	hpet_table = acpi_find_sdt("HPET", 0);
	if (!hpet_table) {
		PANIC("D requires a HPET to be installed");
	}
	hpet = (struct Hpet *)(hpet_table->address.Address + MEM_PHYS_OFFSET);

	clk = hpet->general_capabilities >> 32;

	mmoutq(&hpet->general_configuration, 0);
	mmoutq(&hpet->main_counter_value, 0);
	mmoutq(&hpet->general_configuration, 1);
}

uint64_t hpet_counter_value(void) {
	return mminq(&hpet->main_counter_value);
}

void hpet_usleep(uint64_t us) {
	uint64_t target = hpet_counter_value() + (us * 1000000000) / clk;
	while (hpet_counter_value() < target)
		;
}
