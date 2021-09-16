/*
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

#include "pci.h"
#include "../cpu/ports.h"
#include "../klibc/printf.h"

static uint32_t make_pci_address(uint32_t bus, uint32_t slot, uint32_t function,
								 uint16_t offset) {
	return ((bus << 16) | (slot << 11) | (function << 8) | (offset & 0xFFFC) |
			(1u << 31));
}

uint32_t pci_read(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function,
				  uint16_t offset, uint8_t access_size) {
	(void)seg;
	port_dword_out(0xCF8, make_pci_address(bus, slot, function, offset));
	switch (access_size) {
		case 1:
			return port_byte_in(0xCFC + (offset & 3));
		case 2:
			return port_word_in(0xCFC + (offset & 2));
		case 4:
			return port_dword_in(0xCFC);
		default:
			printf("PCI unknown access size: %hhu\n", access_size);
			return 0;
	}
}

void pci_write(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function,
			   uint16_t offset, uint32_t value, uint8_t access_size) {
	(void)seg;
	port_dword_out(0xCF8, make_pci_address(bus, slot, function, offset));
	switch (access_size) {
		case 1:
			port_byte_out(0xCFC + (offset & 3), value);
			break;
		case 2:
			port_word_out(0xCFC + (offset & 2), value);
			break;
		case 4:
			port_dword_out(0xCFC, value);
			break;
		default:
			printf("PCI unknown access size: %hhu\n", access_size);
			break;
	}
}
