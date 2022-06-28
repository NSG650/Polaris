#ifndef SMP_H
#define SMP_H

#include <limine.h>
#include <stddef.h>
#include <stdint.h>

void smp_init(struct limine_smp_response *smp_info);

#endif
