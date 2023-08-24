#ifndef PRCB_H
#define PRCB_H

#include <stddef.h>
#include <stdint.h>
#if defined(__x86_64__)
#include <sys/gdt.h>
#endif
#include <klibc/vec.h>
#include <sched/sched.h>

struct prcb {
	uint64_t cpu_number;
	uint64_t kernel_stack;
	uint64_t user_stack;
	struct thread *running_thread;
	uint64_t thread_index;
#if defined(__x86_64__)
	struct tss cpu_tss;
	uint32_t lapic_id;
	size_t fpu_storage_size;
	void (*fpu_save)(void *);
	void (*fpu_restore)(void *);
#endif
};

extern struct prcb *prcbs;

#if defined(__x86_64__)

#define prcb_return_current_cpu()               \
	({                                          \
		uint64_t cpu_number;                    \
		asm volatile("mov %0, qword ptr gs:[0]" \
					 : "=r"(cpu_number)         \
					 :                          \
					 : "memory");               \
		&prcbs[cpu_number];                     \
	})

#endif

size_t prcb_return_installed_cpus(void);

#endif
