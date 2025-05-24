/*
 * Copyright 2021 - 2023 Misha
 * Copyright 2021 - 2023 NSG650
 * Copyright 2021 - 2023 Neptune
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

#ifndef HPET_H
#define HPET_H

#include <fw/acpi.h>

struct hpet_table {
	acpi_header_t header;
	uint8_t hardware_rev_id;
	uint8_t comparator_count : 5;
	uint8_t counter_size : 1;
	uint8_t reserved : 1;
	uint8_t legacy_replacement : 1;
	uint16_t pci_vendor_id;
	acpi_gas_t address;
	uint8_t hpet_number;
	uint16_t minimum_tick;
	uint8_t page_protection;
} __attribute__((packed));

struct hpet {
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

uint64_t hpet_counter_value(void);
void hpet_sleep(uint64_t us);

#endif
