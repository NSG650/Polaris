#ifndef MADT_H
#define MADT_H

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

#include <fw/acpi.h>
#include <klibc/vec.h>

struct madt {
	acpi_header_t sdt;
	uint32_t local_controller_addr;
	uint32_t flags;
	char madt_entries_begin[];
} __attribute__((packed));

struct madt_header {
	uint8_t type;
	uint8_t length;
} __attribute__((packed));

struct madt_lapic {
	struct madt_header madt_header;
	uint8_t processor_id;
	uint8_t apic_id;
	uint32_t flags;
} __attribute__((packed));

struct madt_ioapic {
	struct madt_header madt_header;
	uint8_t apic_id;
	uint8_t reserved;
	uint32_t addr;
	uint32_t gsib;
} __attribute__((packed));

struct madt_iso {
	struct madt_header madt_header;
	uint8_t bus_source;
	uint8_t irq_source;
	uint32_t gsi;
	uint16_t flags;
} __attribute__((packed));

struct madt_nmi {
	struct madt_header madt_header;
	uint8_t processor;
	uint16_t flags;
	uint8_t lint;
} __attribute__((packed));

typedef vec_t(struct madt_lapic *) lapic_vec_t;
typedef vec_t(struct madt_ioapic *) ioapic_vec_t;
typedef vec_t(struct madt_iso *) iso_vec_t;
typedef vec_t(struct madt_nmi *) nmi_vec_t;

extern struct madt *madt;
extern lapic_vec_t madt_local_apics;
extern ioapic_vec_t madt_io_apics;
extern iso_vec_t madt_isos;
extern nmi_vec_t madt_nmis;

uintptr_t acpi_get_lapic(void);
void madt_init(void);

#endif
