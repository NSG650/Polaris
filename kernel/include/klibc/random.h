#ifndef RANDOM_H
#define RANDOM_H

#include <stddef.h>
#include <stdint.h>

extern uint64_t (*random_get_seed)(void);

void randdev_init(void);
uint64_t random(void);
void random_set_seed(uint64_t seed);

#endif
