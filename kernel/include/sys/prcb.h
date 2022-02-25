#ifndef PRCB_H
#define PRCB_H

#include <stdint.h>
#include <stddef.h>

struct prcb {
	uint8_t cpu_number;
	char name[3];
};

extern struct prcb **prcbs;

struct prcb *prcb_return_current_cpu(void);
uint64_t prcb_return_installed_cpus(void);

#endif
