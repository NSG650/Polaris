#include <asm/asm.h>
#include <fw/madt.h>
#include <klibc/vec.h>
#include <stdbool.h>
#include <sys/apic.h>
#include <debug/debug.h>

extern bool is_smp;

/*
 * Copyright 2021, 2022 NSG650
 * Copyright 2021, 2022 Sebastian
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
	for (int i = 0; i < madt_local_apics.length; i++) {
		struct madt_lapic *lapic = madt_local_apics.data[i];
		if (lapic_get_id() == lapic->apic_id)
			continue;
		apic_send_ipi(lapic->apic_id, 0xff);
	}
}
