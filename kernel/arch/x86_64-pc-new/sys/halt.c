#include <asm/asm.h>
#include <debug/debug.h>
#include <fw/madt.h>
#include <klibc/vec.h>
#include <stdbool.h>
#include <sys/apic.h>
#include <sys/halt.h>

extern bool is_smp;
bool is_halting = false;
uint8_t is_pausing = false;

/*
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

void halt_current_cpu(void) {
	for (;;) {
		cli();
		halt();
	}
}

void halt_other_cpus(void) {
	if (!is_smp)
		return;

	is_halting = true;

	uint64_t icr = 0;
	icr |= (0b100) << 8; // set delivery mode to nmi
	icr |= (0b11) << 18; // set destination shorthand to all excluding self
	lapic_write(0x300, icr);
	lapic_write(0x310, icr >> 32);
}

void pause_other_cpus(void) {
	if (!is_smp)
		return;

	is_pausing = 0;
	is_pausing |= PAUSING;

	uint64_t icr = 0;
	icr |= (0b100) << 8;
	icr |= (0b11) << 18;
	lapic_write(0x300, icr);
	lapic_write(0x310, icr >> 32);
}

void unpause_other_cpus(void) {
	if (!is_smp)
		return;

	is_pausing = 0;
	is_pausing |= UNPAUSING;

	uint64_t icr = 0;
	icr |= (0b100) << 8;
	icr |= (0b11) << 18;
	lapic_write(0x300, icr);
	lapic_write(0x310, icr >> 32);
}
