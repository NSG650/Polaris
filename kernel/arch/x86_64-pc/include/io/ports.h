#ifndef PORTS_H
#define PORTS_H

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

#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) {
	asm volatile("out %0, %1\n\t" : : "d"(port), "a"(val) : "memory");
}

static inline void outw(uint16_t port, uint16_t val) {
	asm volatile("out %0, %1\n\t" : : "d"(port), "a"(val) : "memory");
}

static inline void outd(uint16_t port, uint32_t val) {
	asm volatile("out %0, %1\n\t" : : "d"(port), "a"(val) : "memory");
}

static inline uint8_t inb(uint16_t port) {
	uint8_t ret;
	asm volatile("in %0, %1\n\t" : "=a"(ret) : "d"(port) : "memory");
	return ret;
}

static inline uint16_t inw(uint16_t port) {
	uint16_t ret;
	asm volatile("in %0, %1\n\t" : "=a"(ret) : "d"(port) : "memory");
	return ret;
}

static inline uint32_t ind(uint16_t port) {
	uint32_t ret;
	asm volatile("in %0, %1\n\t" : "=a"(ret) : "d"(port) : "memory");
	return ret;
}

#endif
