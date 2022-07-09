#ifndef PRCB_H
#define PRCB_H

#include <stddef.h>
#include <stdint.h>
#if defined(__x86_64__)
#include "../../arch/x86_64/include/sys/gdt.h"
#endif
#include <klibc/vec.h>
#include <sched/sched.h>

struct prcb {
	uint64_t cpu_number;
	uint64_t kernel_stack;
	uint64_t user_stack;
	char name[3];
	struct thread *running_thread;
	uint64_t thread_index;
#if defined(__x86_64__)
	struct tss cpu_tss;
#endif
};

typedef vec_t(struct prcb *) prcb_vec_t;

extern prcb_vec_t prcbs;

struct prcb *prcb_return_current_cpu(void);
uint64_t prcb_return_installed_cpus(void);

#endif
