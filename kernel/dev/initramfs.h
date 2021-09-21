#ifndef INITRAMFS_H
#define INITRAMFS_H

#include <stdbool.h>
#include <stivale2.h>

bool initramfs_init(struct stivale2_struct_tag_modules *modules_tag);

#endif
