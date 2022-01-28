#ifndef PROCESS_H
#define PROCESS_H

/*
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

#include "../klibc/vec.h"
#include "sched_types.h"
#include "thread.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define KSTACK_SIZE 32768

extern process_vec_t ptable;

void process_create(char *name, uintptr_t addr, uint64_t args,
					enum priority priority);
void process_init(uintptr_t addr, uint64_t args);
void process_block(enum block_on reason);
void process_exit(void);
uint32_t process_fork(uint8_t timeslice);
void process_unblock(struct process *proc);
void process_sleep(size_t sleep_ticks);
int32_t process_kill(uint32_t pid);
uint32_t process_wait(void);

#endif
