/*
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

#include "ports.h"

uint8_t port_byte_in(uint16_t port) {
	uint8_t ret;
	asm volatile("in %0, %1\n\t" : "=a"(ret) : "d"(port) : "memory");
	return ret;
}

void port_byte_out(uint16_t port, uint8_t data) {
	asm volatile("out %0, %1\n\t" : : "d"(port), "a"(data) : "memory");
}

uint16_t port_word_in(uint16_t port) {
	uint16_t ret;
	asm volatile("in %0, %1\n\t" : "=a"(ret) : "d"(port) : "memory");
	return ret;
}

void port_word_out(uint16_t port, uint16_t data) {
	asm volatile("out %0, %1\n\t" : : "d"(port), "a"(data) : "memory");
}

uint32_t port_dword_in(uint16_t port) {
	uint32_t ret;
	asm volatile("in %0, %1\n\t" : "=a"(ret) : "d"(port) : "memory");
	return ret;
}

void port_dword_out(uint16_t port, uint32_t data) {
	asm volatile("out %0, %1\n\t" : : "d"(port), "a"(data) : "memory");
}
