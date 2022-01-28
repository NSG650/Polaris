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

#include "rand.h"
#include "../sys/clock.h"
#include <cpuid.h>

// Mersenne Twister
// Based on mt19937ar.c found here:
// http://www.math.sci.hiroshima-u.ac.jp/m-mat/MT/MT2002/CODES/mt19937ar.c

#define W 64
#define N 312
#define M 156
#define R 31

#define A 0xB5026F5AA96619E9ULL
#define U 29
#define D 0x5555555555555555ULL
#define S 17
#define B 0x71D67FFFEDA60000ULL
#define T 37
#define C 0xFFF7EEE000000000ULL
#define L 43
#define F 6364136223846793005

#define MASK_LOW ((1ULL << R) - 1)
#define MASK_UPP (~MASK_LOW)

static uint64_t state[N];
static uint32_t index = N + 1;

static void twist(void) {
	for (uint32_t i = 0; i < N; ++i) {
		uint64_t x = (state[i] & MASK_UPP) + (state[(i + 1) % N] & MASK_LOW);
		uint64_t xa = x >> 1;
		if (x & 1) {
			xa ^= A;
		}
		state[i] = state[(i + M) % N] ^ xa;
	}
	index = 0;
}

// Generate number that can be used as seed
uint64_t get_rdseed(void) {
	uint64_t rdseed = 0;
	uint32_t a = 0, b = 0, c = 0, d = 0;

	// Use rdseed when possible
	__get_cpuid(7, &a, &b, &c, &d);
	if (b & bit_RDSEED) {
		asm("rdseed %0" : "=r"(rdseed));
	} else {
		__get_cpuid(1, &a, &b, &c, &d);
		// Use rdrand otherwise
		if (c & bit_RDRND) {
			asm("rdrand %0" : "=r"(rdseed));
		} else {
			// Use UNIX time as last resort
			rdseed = get_unix_timestamp();
		}
	}

	return rdseed;
}

void srand(uint64_t seed) {
	state[0] = seed;
	index = N;
	for (uint32_t i = 1; i < N; ++i) {
		state[i] = F * (state[i - 1] ^ (state[i - 1] >> (W - 2))) + i;
	}
}

uint64_t rand(void) {
	if (index >= N) {
		if (index > N) {
			srand(5489);
		}
		twist();
	}

	uint64_t y = state[index];
	y ^= (y >> U) & D;
	y ^= (y << S) & B;
	y ^= (y << T) & C;
	y ^= y >> L;

	++index;
	return y;
}
