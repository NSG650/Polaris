#ifndef PIT_H
#define PIT_H

#include <stdint.h>

#define PIT_DIVIDEND ((uint64_t)1193182)

uint16_t pit_counter_value(void);
void pit_set_reload_value(uint16_t new_count);
void pit_set_frequency(uint64_t frequency);
void pit_init();

#endif