// Is TAR a FS?
// I mean early UNIX systems used TAR tape drives as their root fs
// Maybe should work out for now
// Pretty funny we are gonna use a NVMe drive as a tape drive lol

#include "tarfs.h"
#include <debug/debug.h>
#include <klibc/mem.h>
#include <klibc/module.h>
#include <mm/pmm.h>
#include <mm/vmm.h>

void *memcpy(void *dest, const void *src, size_t n) {
	uint8_t *pdest = (uint8_t *)dest;
	const uint8_t *psrc = (const uint8_t *)src;

	for (size_t i = 0; i < n; i++) {
		pdest[i] = psrc[i];
	}

	return dest;
}

void *memset(void *b, int c, size_t len) {
	size_t i = 0;
	unsigned char *p = b;
	while (len > 0) {
		*p = c;
		p++;
		len--;
	}
	return b;
}

int memcmp(const void *s1, const void *s2, size_t n) {
	const uint8_t *p1 = (const uint8_t *)s1;
	const uint8_t *p2 = (const uint8_t *)s2;

	for (size_t i = 0; i < n; i++) {
		if (p1[i] != p2[i]) {
			return p1[i] < p2[i] ? -1 : 1;
		}
	}

	return 0;
}

static uint64_t octal_to_int(const char *s) {
	uint64_t ret = 0;
	while (*s) {
		ret *= 8;
		ret += *s - '0';
		s++;
	}
	return ret;
}

void remove_end_char(char *string, char c) {
	size_t l = strlen(string);
	if (string[l - 1] == c) {
		string[l - 1] = '\0';
	}
}

char *get_parent_dir_from_full_path(char *path) {
	size_t l = strlen(path) + 1;
	size_t i = l;
	char *dup = strdup(path);
	while (dup[i] != '/') {
		i--;
	}
	dup[i] = '\0';
	return dup;
}

char *get_file_name_from_full_path(char *path) {
	size_t l = strlen(path) + 1;
	bool found = false;
	while (l) {
		if (path[l--] == '/') {
			found = true;
			break;
		}
	}
	return found ? &path[l + 2] : path;
}

static ssize_t tarfs_resource_read(struct resource *_this,
								   struct f_description *description, void *buf,
								   off_t offset, size_t count) {
	(void)description;
	struct tarfs_resource *this = (struct tarfs_resource *)_this;
	spinlock_acquire_or_wait(&this->res.lock);

	ssize_t actual_count = count;

	if ((off_t)(offset + count) >= this->res.stat.st_size) {
		actual_count = count - ((offset + count) - this->res.stat.st_size);
	}

	struct resource *device_resource = this->fs->device->resource;

	if (device_resource->read(device_resource, NULL, buf, offset + this->offset,
							  actual_count) < 0) {
		actual_count = -1;
	}

	spinlock_drop(&this->res.lock);
	return actual_count;
}

static ssize_t tarfs_resource_write(struct resource *_this,
									struct f_description *description,
									const void *buf, off_t offset,
									size_t count) {
	return -1;
}

static void *tarfs_resource_mmap(struct resource *_this, size_t file_page,
								 int flags) {
	struct tarfs_resource *this = (struct tarfs_resource *)_this;
	spinlock_acquire_or_wait(&this->res.lock);

	void *ret = pmm_alloc(1);
	if (ret != NULL) {
		if (this->res.read(&this->res, NULL,
						   (void *)((uintptr_t)ret + MEM_PHYS_OFFSET),
						   file_page * PAGE_SIZE, PAGE_SIZE) < 0) {
			pmm_free(ret, 1);
			ret = NULL;
		}
	}

	spinlock_drop(&this->res.lock);
	return ret;
}

static bool tarfs_resource_truncate(struct resource *this_,
									struct f_description *description,
									size_t length) {
	return false;
}

static struct vfs_node *tarfs_create(struct vfs_filesystem *_this,
									 struct vfs_node *parent, const char *name,
									 int mode) {
	return NULL;
}

static void tarfs_populate(struct vfs_filesystem *_this,
						   struct vfs_node *node) {
	struct tarfs *this = (struct tarfs *)_this;
	struct resource *device_resource = this->device->resource;
	struct ustar_header *current_file = kmalloc(512);
	off_t offset = 0;
	if (this->root) {
		for (;;) {
			device_resource->read(device_resource, NULL, current_file, offset,
								  512);
			if (strncmp(current_file->signature, "ustar", 5)) {
				break;
			}

			char *name = current_file->name;

			if (!strcmp(name, "./") || !strcmp(name, "../") ||
				!strcmp(name, ".") || !strcmp(name, "..")) {
				continue;
			}
			size_t mode = octal_to_int(current_file->mode);
			size_t size = octal_to_int(current_file->size);

			struct vfs_node *fnode = NULL;
			struct vfs_node *parent = node;
			struct tarfs_resource *res =
				resource_create(sizeof(struct tarfs_resource));

			if (res == NULL) {
				kprintf("tarfs: skipping over %s\n", name);
				continue;
			}

			res->res.write = tarfs_resource_write;
			res->res.read = tarfs_resource_read;
			res->res.mmap = tarfs_resource_mmap;
			res->res.truncate = tarfs_resource_truncate;
			res->offset = offset + 512;
			res->fs = this;

			res->res.stat.st_mode = mode;
			res->res.stat.st_size = size;
			res->res.stat.st_ino = this->inode_count++;
			res->res.stat.st_nlink = 1;
			res->res.stat.st_dev = device_resource->stat.st_rdev;
			res->res.stat.st_blksize = 512;
			res->res.stat.st_blocks = size / 512;
			res->res.refcount = 1;

			switch (current_file->type) {
				case USTAR_REGULAR:
				case USTAR_NORMAL:
				case USTAR_CONTIGOUS: {
					char *parent_path = get_parent_dir_from_full_path(name);
					struct vfs_node *p = NULL;
					if (HASHMAP_SGET(&this->directories, p, parent_path)) {
						parent = p;
					}
					fnode = vfs_create_node(
						(struct vfs_filesystem *)this, parent,
						get_file_name_from_full_path(name), false);
					kfree(parent_path);
					res->res.stat.st_mode = mode | S_IFREG;
					res->res.can_mmap = true;
					break;
				}
				case USTAR_DIRECTORY: {
					remove_end_char(name, '/');
					char *parent_path = get_parent_dir_from_full_path(name);
					struct vfs_node *p = NULL;
					if (HASHMAP_SGET(&this->directories, p, parent_path)) {
						parent = p;
					}
					fnode = vfs_create_node(
						(struct vfs_filesystem *)this, parent,
						get_file_name_from_full_path(name), true);
					kfree(parent_path);
					res->res.stat.st_mode = mode | S_IFDIR;
					vfs_create_dotentries(fnode, fnode->parent);
					HASHMAP_SINSERT(&this->directories, name, fnode);
					break;
				}
			}

			fnode->filesystem = _this;
			fnode->resource = (struct resource *)res;
			fnode->populated = false;

			HASHMAP_SINSERT(&fnode->parent->children, name, fnode);
			offset += 512 + ALIGN_UP(size, 512);
			this->total_size += size;
		}
		this->root = false;
	}
	node->resource->stat.st_blocks = this->total_size / 512;
	node->resource->stat.st_size = this->total_size;
	node->populated = true;
	kfree(current_file);
}

static struct vfs_filesystem *tarfs_instantiate(void) {
	struct tarfs *f = kmalloc(sizeof(struct tarfs));
	if (f == NULL) {
		return NULL;
	}

	return (struct vfs_filesystem *)f;
}

static struct vfs_node *tarfs_mount(struct vfs_node *parent, const char *name,
									struct vfs_node *device) {
	struct tarfs *f = (struct tarfs *)tarfs_instantiate();
	f->device = device;
	f->fs.populate = tarfs_populate;
	f->fs.create = tarfs_create;
	f->root = true;
	f->directories = (typeof(f->directories))HASHMAP_INIT(256);

	struct vfs_node *node =
		vfs_create_node((struct vfs_filesystem *)f, parent, name, true);

	if (f == NULL || node == NULL) {
		return NULL;
	}

	f->inode_count = 3;

	struct tarfs_resource *res = resource_create(sizeof(struct tarfs_resource));
	res->res.read = tarfs_resource_read;
	res->res.write = tarfs_resource_write;

	res->res.stat.st_blocks = device->resource->stat.st_size / 512;
	res->res.stat.st_size = device->resource->stat.st_size;
	res->res.stat.st_blksize = 512;
	res->res.stat.st_dev = device->resource->stat.st_rdev;
	res->res.stat.st_mode = 0644 | S_IFDIR;
	res->res.stat.st_nlink = 1;
	res->res.stat.st_ino = 2;

	res->fs = f;

	node->resource = (struct resource *)res;
	node->filesystem = (struct vfs_filesystem *)f;

	return node;
}

uint64_t driver_entry(struct module *driver_module) {
	driver_module->exit = NULL;
	vfs_add_filesystem(tarfs_mount, "tarfs");
	return 0;
}
