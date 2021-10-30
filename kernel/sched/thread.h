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

#ifndef THREAD_H
#define THREAD_H

#include "../klibc/vec.h"
#include "process.h"
#include "sched_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TSTACK_SIZE 32768

void thread_init(uintptr_t addr, uint64_t args, struct process *proc);
void thread_create(uintptr_t addr, uint64_t args);
void thread_block(enum block_on reason);
void thread_exit(void);
void thread_unblock(struct thread *thread);
void thread_sleep(size_t sleep_ticks);

#endif