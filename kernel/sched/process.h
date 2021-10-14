#ifndef PROCESS_H
#define PROCESS_H

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

#include "../klibc/vec.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define KSTACK_SIZE 32768

struct process_context {
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

enum process_priority { LOW = 0, NORMAL, HIGH };

enum process_block_on { NOTHING, ON_SLEEP, ON_WAIT, ON_LOCK };

enum process_state { UNUSED, INITIAL, READY, RUNNING, BLOCKED, TERMINATED };

struct process {
	char name[64];
	uint32_t pid;
	struct process_context *context;
	enum process_state state;
	enum process_block_on block_on;
	enum process_priority priority;
	uint8_t *kstack;
	struct process *parent;
	bool killed;
	uint8_t timeslice;
	size_t target_tick;
};

typedef vec_t(struct process *) process_vec_t;
extern process_vec_t ptable;

void process_create(char *name, uintptr_t addr, uint64_t args,
					enum process_priority priority);
void process_init(uintptr_t addr, uint64_t args);
void process_block(enum process_block_on reason);
void process_exit(void);
uint32_t process_fork(uint8_t timeslice);
void process_unblock(struct process *proc);
void process_sleep(size_t sleep_ticks);
int32_t process_kill(uint32_t pid);
uint32_t process_wait(void);

#endif
