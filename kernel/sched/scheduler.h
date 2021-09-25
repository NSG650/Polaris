#ifndef SCHEDULER_H
#define SCHEDULER_H

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

#include "process.h"

struct cpu_state {
	struct process_context *scheduler;
	struct process *running_proc;
};

struct process *running_proc(void);
void yield_to_scheduler(void);
void sched_init(void);

#endif
