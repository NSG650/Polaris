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

#include <asm/asm.h>
#include <io/mmio.h>

void outmmb(void *addr, uint8_t value) {
	asm volatile("mov %0, %1\n\t"
				 : "=m"(BYTE_PTR(addr))
				 : "r"(value)
				 : "memory");
}

void outmmw(void *addr, uint16_t value) {
	asm volatile("mov %0, %1\n\t"
				 : "=m"(WORD_PTR(addr))
				 : "r"(value)
				 : "memory");
}

void outmmdw(void *addr, uint32_t value) {
	asm volatile("mov %0, %1\n\t"
				 : "=m"(DWORD_PTR(addr))
				 : "r"(value)
				 : "memory");
}

void outmmqw(void *addr, uint64_t value) {
	asm volatile("mov %0, %1\n\t"
				 : "=m"(QWORD_PTR(addr))
				 : "r"(value)
				 : "memory");
}

uint8_t inmmb(void *addr) {
	uint8_t ret;
	asm volatile("mov %0, %1\n\t" : "=r"(ret) : "m"(BYTE_PTR(addr)) : "memory");
	return ret;
}

uint16_t inmmw(void *addr) {
	uint16_t ret;
	asm volatile("mov %0, %1\n\t" : "=r"(ret) : "m"(WORD_PTR(addr)) : "memory");
	return ret;
}

uint32_t inmmdw(void *addr) {
	uint32_t ret;
	asm volatile("mov %0, %1\n\t"
				 : "=r"(ret)
				 : "m"(DWORD_PTR(addr))
				 : "memory");
	return ret;
}

uint64_t inmmqw(void *addr) {
	uint64_t ret;
	asm volatile("mov %0, %1\n\t"
				 : "=r"(ret)
				 : "m"(QWORD_PTR(addr))
				 : "memory");
	return ret;
}
