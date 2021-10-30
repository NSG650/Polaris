/*
 * Copyright 2021 NSG650
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

#ifndef SCHED_TYPES
#define SCHED_TYPES

#include <stddef.h>
#include <stdint.h>

struct cpu_context {
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t r11;
	uint64_t r10;
	uint64_t r9;
	uint64_t r8;
	uint64_t rsi;
	uint64_t rdi;
	uint64_t rbp;
	uint64_t rdx;
	uint64_t rcx;
	uint64_t rbx;
	uint64_t rax;
	uint64_t rip;
} __attribute__((packed));

enum priority { LOW = 0, NORMAL, HIGH };

enum block_on { NOTHING, ON_SLEEP, ON_WAIT, ON_LOCK };

enum state { UNUSED, INITIAL, READY, RUNNING, BLOCKED, TERMINATED };

struct thread {
	uint32_t tid;
	uint8_t *tstack;
	enum state state_t;
	enum block_on block_t;
	struct cpu_context *context;
	size_t target_tick;
	bool killed;
};

typedef vec_t(struct thread *) thread_vec_t;

struct process {
	char name[128];
	uint32_t pid;
	enum state state;
	enum block_on block_on;
	enum priority priority;
	struct process *parent;
	thread_vec_t ttable;
	bool killed;
	uint8_t timeslice;
	size_t target_tick;
};

typedef vec_t(struct process *) process_vec_t;

#endif