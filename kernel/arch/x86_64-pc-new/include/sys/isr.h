#ifndef ISR_H
#define ISR_H

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

#include <reg.h>

#define PASTER(x, y) x##y
#define EVALUATOR(x, y) PASTER(x, y)
// This first macro makes functions like "void isr0(void)"
// But the number is summed up on each call
#define ONE EVALUATOR(void isr, __COUNTER__)(void)
// Different granularities
#define TWO \
	ONE;    \
	ONE
#define TEN \
	TWO;    \
	TWO;    \
	TWO;    \
	TWO;    \
	TWO
#define TWENTY \
	TEN;       \
	TEN
#define HUNDRED \
	TWENTY;     \
	TWENTY;     \
	TWENTY;     \
	TWENTY;     \
	TWENTY
// Define 255 ISRs based on previous granularities
#define DEFISR \
	HUNDRED;   \
	HUNDRED;   \
	TWENTY;    \
	TWENTY;    \
	TEN;       \
	TWO;       \
	TWO;       \
	TWO

DEFISR;

typedef void (*event_handlers_t)(registers_t *);

void isr_install(void);
extern void isr_handle(registers_t *r);
void isr_register_handler(int n, void *handler);

#endif
