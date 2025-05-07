#include <debug/debug.h>
#include <errno.h>
#include <fs/devtmpfs.h>
#include <fs/vfs.h>
#include <klibc/misc.h>
#include <klibc/resource.h>
#include <klibc/time.h>
#include <locks/spinlock.h>
#include <mm/mmap.h>
#include <mm/pmm.h>
#include <mm/slab.h>
#include <mm/vmm.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

struct devtmpfs_resource {
	struct resource;

	void *data;
	size_t capacity;
};

struct devtmpfs {
	struct vfs_filesystem;

	uint64_t dev_id;
	uint64_t inode_counter;
};

static struct vfs_filesystem *devtmpfs = NULL;
static struct vfs_node *devtmpfs_root = NULL;

static ssize_t devtmpfs_resource_read(struct resource *_this,
									  struct f_description *description,
									  void *buf, off_t offset, size_t count) {
	(void)description;

	struct devtmpfs_resource *this = (struct devtmpfs_resource *)_this;

	spinlock_acquire_or_wait(&this->lock);

	size_t actual_count = count;

	if ((off_t)(offset + count) >= this->stat.st_size) {
		actual_count = count - ((offset + count) - this->stat.st_size);
	}

	memcpy(buf, this->data + offset, actual_count);
	spinlock_drop(&this->lock);

	return actual_count;
}

static ssize_t devtmpfs_resource_write(struct resource *_this,
									   struct f_description *description,
									   const void *buf, off_t offset,
									   size_t count) {
	(void)description;

	ssize_t ret = -1;
	struct devtmpfs_resource *this = (struct devtmpfs_resource *)_this;

	spinlock_acquire_or_wait(&this->lock);

	if (offset + count >= this->capacity) {
		size_t new_capacity = this->capacity;
		while (offset + count >= new_capacity) {
			new_capacity *= 2;
		}

		void *new_data = kmalloc(new_capacity);
		if (new_data == NULL) {
			errno = ENOMEM;
			goto fail;
		}

		memcpy(new_data, this->data, this->capacity);
		kfree(this->data);

		this->data = new_data;
		this->capacity = new_capacity;
	}

	memcpy(this->data + offset, buf, count);

	if ((off_t)(offset + count) >= this->stat.st_size) {
		this->stat.st_size = (off_t)(offset + count);
		this->stat.st_blocks =
			DIV_ROUNDUP(this->stat.st_size, this->stat.st_blksize);
	}

	ret = count;

fail:
	spinlock_drop(&this->lock);
	return ret;
}

static void *devtmpfs_resource_mmap(struct resource *_this, size_t file_page,
									int flags) {
	struct devtmpfs_resource *this = (struct devtmpfs_resource *)_this;

	spinlock_acquire_or_wait(&this->lock);

	void *ret = NULL;
	if ((flags & MAP_SHARED) != 0) {
		ret = (this->data + file_page * PAGE_SIZE) - MEM_PHYS_OFFSET;
	} else {
		ret = pmm_alloc(1);
		if (ret == NULL) {
			goto cleanup;
		}

		memcpy(ret + MEM_PHYS_OFFSET, this->data + file_page * PAGE_SIZE,
			   PAGE_SIZE);
	}

cleanup:
	spinlock_drop(&this->lock);
	return ret;
}

static bool devtmpfs_truncate(struct resource *this_,
							  struct f_description *description,
							  size_t length) {
	(void)description;

	struct devtmpfs_resource *this = (struct devtmpfs_resource *)this_;

	if (length > this->capacity) {
		size_t new_capacity = this->capacity;
		while (new_capacity < length) {
			new_capacity *= 2;
		}

		void *new_data = kmalloc(new_capacity);
		if (new_data == NULL) {
			errno = ENOMEM;
			goto fail;
		}

		memcpy(new_data, this->data, this->capacity);
		kfree(this->data);

		this->data = new_data;
		this->capacity = new_capacity;
	}

	this->stat.st_size = (off_t)length;
	this->stat.st_blocks =
		DIV_ROUNDUP(this->stat.st_size, this->stat.st_blksize);

	return true;

fail:
	return false;
}

static inline struct devtmpfs_resource *
create_devtmpfs_resource(struct devtmpfs *this, int mode) {
	struct devtmpfs_resource *resource =
		resource_create(sizeof(struct devtmpfs_resource));
	if (resource == NULL) {
		return resource;
	}

	if (S_ISREG(mode)) {
		resource->capacity = 4096;
		resource->data = kmalloc(resource->capacity);
		resource->can_mmap = true;
	}

	resource->read = devtmpfs_resource_read;
	resource->write = devtmpfs_resource_write;
	resource->mmap = devtmpfs_resource_mmap;
	resource->truncate = devtmpfs_truncate;

	resource->stat.st_size = 0;
	resource->stat.st_blocks = 0;
	resource->stat.st_blksize = 512;
	resource->stat.st_dev = this->dev_id;
	resource->stat.st_ino = this->inode_counter++;
	resource->stat.st_mode = mode;
	resource->stat.st_nlink = 1;

	resource->stat.st_atim = time_realtime;
	resource->stat.st_ctim = time_realtime;
	resource->stat.st_mtim = time_realtime;

	return resource;
}

static inline struct vfs_filesystem *devtmpfs_instantiate(void);

static struct vfs_node *devtmpfs_mount(struct vfs_node *parent,
									   const char *name,
									   struct vfs_node *source) {
	(void)parent;
	(void)name;
	(void)source;
	return devtmpfs_root;
}

static struct vfs_node *devtmpfs_create(struct vfs_filesystem *_this,
										struct vfs_node *parent,
										const char *name, int mode) {
	struct devtmpfs *this = (struct devtmpfs *)_this;
	struct vfs_node *new_node = NULL;
	struct devtmpfs_resource *resource = NULL;

	new_node = vfs_create_node(_this, parent, name, S_ISDIR(mode));
	if (new_node == NULL) {
		goto fail;
	}

	resource = create_devtmpfs_resource(this, mode);
	if (resource == NULL) {
		goto fail;
	}

	new_node->resource = (struct resource *)resource;
	return new_node;

fail:
	if (new_node != NULL) {
		kfree(new_node); // TODO: Use vfs_destroy_node
	}
	if (resource != NULL) {
		kfree(resource);
	}

	return NULL;
}

static struct vfs_node *devtmpfs_symlink(struct vfs_filesystem *_this,
										 struct vfs_node *parent,
										 const char *name, const char *target) {
	struct devtmpfs *this = (struct devtmpfs *)_this;
	struct vfs_node *new_node = NULL;
	struct devtmpfs_resource *resource = NULL;

	new_node = vfs_create_node(_this, parent, name, false);
	if (new_node == NULL) {
		goto fail;
	}

	resource = create_devtmpfs_resource(this, 0777 | S_IFLNK);
	if (resource == NULL) {
		goto fail;
	}

	new_node->resource = (struct resource *)resource;
	new_node->symlink_target = strdup(target);
	return new_node;

fail:
	if (new_node != NULL) {
		kfree(new_node); // TODO: Use vfs_destroy_node
	}
	if (resource != NULL) {
		kfree(resource);
	}

	return NULL;
}

static struct vfs_node *devtmpfs_link(struct vfs_filesystem *_this,
									  struct vfs_node *parent, const char *name,
									  struct vfs_node *node) {
	if (S_ISDIR(node->resource->stat.st_mode)) {
		errno = EISDIR;
		return NULL;
	}

	struct vfs_node *new_node = vfs_create_node(_this, parent, name, false);
	if (new_node == NULL) {
		return NULL;
	}

	new_node->resource = node->resource;
	return new_node;
}

static inline struct vfs_filesystem *devtmpfs_instantiate(void) {
	struct devtmpfs *new_fs = kmalloc(sizeof(struct devtmpfs));
	if (new_fs == NULL) {
		return NULL;
	}

	new_fs->create = devtmpfs_create;
	new_fs->symlink = devtmpfs_symlink;
	new_fs->link = devtmpfs_link;

	return (struct vfs_filesystem *)new_fs;
}

void devtmpfs_init(void) {
	devtmpfs = devtmpfs_instantiate();
	if (devtmpfs == NULL) {
		panic("Failed to instantiate devtmpfs\n");
	}

	devtmpfs_root = devtmpfs->create(devtmpfs, NULL, "", 0755 | S_IFDIR);
	if (devtmpfs_root == NULL) {
		panic("Failed to create root devtmpfs node\n");
	}

	vfs_add_filesystem(devtmpfs_mount, "devtmpfs");
}

bool devtmpfs_add_device(struct resource *device, const char *name) {
	struct vfs_node *node = vfs_get_node(devtmpfs_root, name, false);
	if (node != NULL) {
		errno = EEXIST;
		return false;
	}

	struct vfs_node *new_node =
		vfs_create_node(devtmpfs, devtmpfs_root, name, false);
	if (new_node == NULL) {
		return false;
	}

	new_node->resource = device;

	struct devtmpfs *fs = (struct devtmpfs *)devtmpfs;
	device->stat.st_dev = fs->dev_id;
	device->stat.st_ino = fs->inode_counter++;
	device->stat.st_nlink = 1;

	device->stat.st_atim = time_realtime;
	device->stat.st_ctim = time_realtime;
	device->stat.st_mtim = time_realtime;

	spinlock_acquire_or_wait(&vfs_lock);
	HASHMAP_SINSERT(&devtmpfs_root->children, name, new_node);
	spinlock_drop(&vfs_lock);
	return new_node;
}
