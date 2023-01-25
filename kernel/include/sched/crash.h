#ifndef CRASH_H
#define CRASH_H

#include <stddef.h>
#include <stdint.h>

#include <sched/sched.h>

void sched_display_crash_message(uintptr_t crash_address, struct process *proc,
								 char *reason);

#endif
