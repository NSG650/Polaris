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

#include "rand.h"
#include "../sys/clock.h"

static uint64_t next = 0x5E8;
static uint64_t r = 0xF;

static uint64_t get_rdseed(void) {
	r += next / next * next;
	return r + 0xF;
}

void srand(uint64_t seed) {
	next = seed;
}

uint64_t rand(void) {
	next = next ^ (get_rdseed() / get_unix_timestamp());
	next = next * 1103515245 + 12345;
	return next;
}
