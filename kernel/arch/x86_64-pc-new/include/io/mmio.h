#ifndef MMIO_H
#define MMIO_H

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

#include <asm/asm.h>
#include <stdint.h>

static inline void mmoutb(void *addr, uint8_t value) {
	asm volatile("mov %0, %1\n\t"
				 : "=m"(BYTE_PTR(addr))
				 : "r"(value)
				 : "memory");
}

static inline void mmoutw(void *addr, uint16_t value) {
	asm volatile("mov %0, %1\n\t"
				 : "=m"(WORD_PTR(addr))
				 : "r"(value)
				 : "memory");
}

static inline void mmoutd(void *addr, uint32_t value) {
	asm volatile("mov %0, %1\n\t"
				 : "=m"(DWORD_PTR(addr))
				 : "r"(value)
				 : "memory");
}

static inline void mmoutq(void *addr, uint64_t value) {
	asm volatile("mov %0, %1\n\t"
				 : "=m"(QWORD_PTR(addr))
				 : "r"(value)
				 : "memory");
}

static inline uint8_t mminb(void *addr) {
	uint8_t ret;
	asm volatile("mov %0, %1\n\t" : "=r"(ret) : "m"(BYTE_PTR(addr)) : "memory");
	return ret;
}

static inline uint16_t mminw(void *addr) {
	uint16_t ret;
	asm volatile("mov %0, %1\n\t" : "=r"(ret) : "m"(WORD_PTR(addr)) : "memory");
	return ret;
}

static inline uint32_t mmind(void *addr) {
	uint32_t ret;
	asm volatile("mov %0, %1\n\t"
				 : "=r"(ret)
				 : "m"(DWORD_PTR(addr))
				 : "memory");
	return ret;
}

static inline uint64_t mminq(void *addr) {
	uint64_t ret;
	asm volatile("mov %0, %1\n\t"
				 : "=r"(ret)
				 : "m"(QWORD_PTR(addr))
				 : "memory");
	return ret;
}

#endif
