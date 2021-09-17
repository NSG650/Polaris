#ifndef MADT_H
#define MADT_H

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

#include "../klibc/dynarray.h"
#include "acpi.h"
#include <stdint.h>

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
	struct madt_header madtHeader;
	uint8_t processor_id;
	uint8_t apic_id;
	uint32_t flags;
} __attribute__((packed));

struct madt_ioapic {
	struct madt_header madtHeader;
	uint8_t apic_id;
	uint8_t reserved;
	uint32_t addr;
	uint32_t gsib;
} __attribute__((packed));

struct madt_iso {
	struct madt_header madtHeader;
	uint8_t bus_source;
	uint8_t irq_source;
	uint32_t gsi;
	uint16_t flags;
} __attribute__((packed));

struct madt_nmi {
	struct madt_header madtHeader;
	uint8_t processor;
	uint16_t flags;
	uint8_t lint;
} __attribute__((packed));

extern struct madt *madt;

DYNARRAY_EXTERN(struct madt_lapic *, madt_local_apics);
DYNARRAY_EXTERN(struct madt_ioapic *, madt_io_apics);
DYNARRAY_EXTERN(struct madt_iso *, madt_isos);
DYNARRAY_EXTERN(struct madt_nmi *, madt_nmis);

uintptr_t acpi_get_lapic(void);
void init_madt(void);

#endif
