#ifndef HPET_H
#define HPET_H

#include <stdint.h>

uint64_t hpet_counter_value(void);
void hpet_init(void);
void hpet_usleep(uint64_t us);

#endif
