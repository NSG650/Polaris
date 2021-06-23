#ifndef PIT_H
#define PIT_H

#include <stdint.h>
#include "reg.h"
#include "isr.h"

void timer_handler(registers_t* r);
void set_pit_freq(uint64_t freq);
void wait(uint64_t ticks);

extern volatile uint64_t global_tick;

#endif
