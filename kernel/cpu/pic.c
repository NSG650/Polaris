/*
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

#include "pic.h"
#include "../acpi/madt.h"
#include "ports.h"

static void pic_disable(void) {
	port_byte_out(0xA1, 0xFF);
	port_byte_out(0x21, 0xFF);
}

void pic_init(void) {
	// There isn't any PIC
	if (!(madt->flags & 1)) {
		return;
	}
	// Remap the PIC
	port_byte_out(0x20, 0x11);
	port_byte_out(0xA0, 0x11);
	port_byte_out(0x21, 0x20);
	port_byte_out(0xA1, 0x28);
	port_byte_out(0x21, 4);
	port_byte_out(0xA1, 2);
	port_byte_out(0x21, 1);
	port_byte_out(0xA1, 1);
	port_byte_out(0x21, 0);
	port_byte_out(0xA1, 0);

	pic_disable();
}
