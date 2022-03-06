#ifndef PRCB_H
#define PRCB_H

#include <stdint.h>
#include <stddef.h>
#if defined(__x86_64__)
#include "../../arch/x86_64/include/sys/gdt.h"
#endif
#include <sched/sched.h>
#include <klibc/vec.h>

struct prcb {
	uint8_t cpu_number;
	char name[3];
	struct thread *running_thread;
	uint64_t thread_index;
#if defined(__x86_64__)
	#endif
};

typedef vec_t(struct prcb *) prcb_vec_t;

extern prcb_vec_t prcbs;

struct prcb *prcb_return_current_cpu(void);
uint64_t prcb_return_installed_cpus(void);

#endif
