#include <fs/vfs.h>
#include <klibc/mem.h>

uint32_t testfs_read(struct file *this, size_t count, size_t offset,
					 uint8_t *out) {
	spinlock_acquire_or_wait(this->lock);
	if (offset + count > (size_t)this->size)
		count -= (offset + count) - this->size;
	memcpy(out, this->data + offset, count);
	spinlock_drop(this->lock);
	return 0;
}

uint32_t testfs_write(struct file *this, size_t count, size_t offset,
					  uint8_t *in) {
	spinlock_acquire_or_wait(this->lock);
	if (offset + count > this->allocated_size) {
		this->allocated_size *= 2;
		krealloc(this->data, this->allocated_size);
	}
	memcpy(this->data + offset, in, count);
	this->size += (offset + count) - this->size;
	spinlock_drop(this->lock);
	return 0;
}

struct file *testfs_open(struct fs_node *node, char *name) {
	for (int i = 0; i < node->files.length; i++) {
		if (!strcmp(node->files.data[i]->name, name))
			return node->files.data[i];
	}
	return NULL;
}

uint32_t testfs_create(struct fs_node *node, char *name) {
	struct file *new = kmalloc(sizeof(struct file));
	new->name = name;
	new->size = 0;
	new->allocated_size = 1024;
	new->uid = 0;
	new->gid = 0;
	new->lock = 0;
	new->read = testfs_read;
	new->write = testfs_write;
	new->data = kmalloc(new->allocated_size);
	vec_push(&node->files, new);
	return 0;
}

struct fs testfs = {
	.name = "testfs", .open = testfs_open, .create = testfs_create};