/*
 * Copyright 2021 NSG650
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef VFS_H
#define VFS_H

#include "../klibc/alloc.h"
#include "../klibc/lock.h"
#include "../klibc/resource.h"
#include "../klibc/types.h"
#include <stdbool.h>

extern lock_t vfs_lock;

struct filesystem {
	const char *name;
	bool needs_backing_device;
	struct vfs_node *(*mount)(struct resource *device);
	struct vfs_node *(*populate)(struct vfs_node *node);
	struct resource *(*open)(struct vfs_node *node, bool new_node, mode_t mode);
	struct resource *(*mkdir)(struct vfs_node *node, mode_t mode);
};

#define VFS_ROOT_INODE ((ino_t)0xffffffffffffffff)

struct vfs_node {
	char name[NAME_MAX];
	struct resource *res;
	void *mount_data;
	dev_t backing_dev_id;
	struct filesystem *fs;
	struct vfs_node *mount_gate;
	struct vfs_node *parent;
	struct vfs_node *child;
	struct vfs_node *next;
};

struct vfs_node *vfs_new_node(struct vfs_node *parent, const char *name);
struct vfs_node *vfs_new_node_deep(struct vfs_node *parent, const char *name);
void vfs_dump_nodes(struct vfs_node *node, const char *parent);
void vfs_get_absolute_path(char *path_ptr, const char *path, const char *pwd);
bool vfs_install_fs(struct filesystem *fs);
bool vfs_mount(const char *source, const char *target, const char *fs);
struct vfs_node *vfs_mkdir(struct vfs_node *parent, const char *name,
						   mode_t mode, bool recurse);
struct resource *vfs_open(const char *path, int oflags, mode_t mode);
bool vfs_stat(const char *path, struct stat *st);

#endif
