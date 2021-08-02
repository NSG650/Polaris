#ifndef HPET_H
#define HPET_H

#include <stdint.h>

void hpet_init(void);
void hpet_usleep(uint64_t us);

#endif
