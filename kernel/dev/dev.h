#ifndef DEV_H
#define DEV_H

#include "../klibc/resource.h"
#include "../klibc/types.h"
#include <stdbool.h>

dev_t dev_new_id(void);
bool dev_add_new(struct resource *device, const char *dev_name);

#endif
