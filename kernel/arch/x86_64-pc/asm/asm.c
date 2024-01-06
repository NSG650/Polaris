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

void halt(void) {
	asm("hlt");
}

void cli(void) {
	asm("cli");
}

void sti(void) {
	asm("sti");
}

void pause(void) {
	asm("pause");
}

void nop(void) {
	asm("nop");
}

void dbgbrk(void) {
	asm("int 3");
}
