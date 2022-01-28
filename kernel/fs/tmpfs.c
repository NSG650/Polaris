/*
 * Copyright 2021, 2022 NSG650
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

#include "tmpfs.h"
#include "../klibc/lock.h"
#include "../klibc/mem.h"
#include "../klibc/resource.h"
#include "vfs.h"
#include <liballoc.h>
#include <stddef.h>

struct tmpfs_resource {
	struct resource res;
	size_t allocated_size;
	char *data;
};

struct tmpfs_mount_data {
	ino_t inode_counter;
};

static struct vfs_node *tmpfs_mount(struct resource *device) {
	(void)device;
	struct vfs_node *mount_gate = kmalloc(sizeof(struct vfs_node));
	mount_gate->fs = &tmpfs;
	struct tmpfs_mount_data *mount_data =
		kmalloc(sizeof(struct tmpfs_mount_data));
	mount_gate->mount_data = mount_data;
	mount_data->inode_counter = 1;
	return mount_gate;
}

static ssize_t tmpfs_read(struct resource *_this, void *buf, off_t off,
						  size_t count) {
	struct tmpfs_resource *this = (void *)_this;
	LOCK(this->res.lock);

	if (off + count > (size_t)this->res.st.st_size)
		count -= (off + count) - this->res.st.st_size;

	memcpy(buf, this->data + off, count);

	UNLOCK(this->res.lock);

	return count;
}

static ssize_t tmpfs_write(struct resource *_this, const void *buf, off_t off,
						   size_t count) {
	struct tmpfs_resource *this = (void *)_this;
	LOCK(this->res.lock);
	if (off + count > this->allocated_size) {
		while (off + count > this->allocated_size)
			this->allocated_size *= 2;

		this->data = krealloc(this->data, this->allocated_size);
	}
	memcpy(this->data + off, buf, count);
	this->res.st.st_size += count;
	UNLOCK(this->res.lock);
	return count;
}

static int tmpfs_close(struct resource *_this) {
	struct tmpfs_resource *this = (void *)_this;
	LOCK(this->res.lock);
	this->res.refcount--;
	UNLOCK(this->res.lock);
	return 0;
}

static struct resource *tmpfs_open(struct vfs_node *node, bool create,
								   mode_t mode) {
	if (!create)
		return NULL;

	struct tmpfs_mount_data *mount_data = node->mount_data;
	struct tmpfs_resource *res = resource_create(sizeof(struct tmpfs_resource));

	res->allocated_size = 4096;
	res->data = kmalloc(res->allocated_size);
	res->res.st.st_dev = node->backing_dev_id;
	res->res.st.st_size = 0;
	res->res.st.st_blocks = 0;
	res->res.st.st_blksize = 512;
	res->res.st.st_ino = mount_data->inode_counter++;
	res->res.st.st_mode = (mode & ~S_IFMT) | S_IFREG;
	res->res.st.st_nlink = 1;
	res->res.close = tmpfs_close;
	res->res.read = tmpfs_read;
	res->res.write = tmpfs_write;

	return (void *)res;
}

static struct resource *tmpfs_mkdir(struct vfs_node *node, mode_t mode) {
	struct tmpfs_mount_data *mount_data = node->mount_data;

	struct resource *res = resource_create(sizeof(struct resource));

	res->st.st_dev = node->backing_dev_id;
	res->st.st_size = 0;
	res->st.st_blocks = 0;
	res->st.st_blksize = 512;
	res->st.st_ino = mount_data->inode_counter++;
	res->st.st_mode = (mode & ~S_IFMT) | S_IFDIR;
	res->st.st_nlink = 1;

	return (void *)res;
}

static struct vfs_node *tmpfs_populate(struct vfs_node *node) {
	(void)node;
	return NULL;
}

struct filesystem tmpfs = {.name = "tmpfs",
						   .needs_backing_device = false,
						   .mount = tmpfs_mount,
						   .open = tmpfs_open,
						   .mkdir = tmpfs_mkdir,
						   .populate = tmpfs_populate};
