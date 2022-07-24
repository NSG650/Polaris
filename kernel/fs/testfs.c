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

uint32_t testfs_delete(struct fs_node *node, char *name) {
	struct file *filex = NULL;
	for (int i = 0; i < node->files.length; i++) {
		if (!strcmp(node->files.data[i]->name, name)) {
			filex = node->files.data[i];
			break;
		}
	}
	if (filex) {
		vec_remove(&node->files, filex);
		kfree(filex->data);
		kfree(filex);
		return 0;
	} else
		return 1;
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
	new->readdir = NULL;
	new->type = S_IFMT &S_IFREG;
	new->data = kmalloc(new->allocated_size);
	vec_push(&node->files, new);
	return 0;
}

struct fs_node *testfs_readdir(struct file *file) {
	if (S_ISDIR(file->type)) {
		return (struct fs_node *)file->data;
	}
	return NULL;
}

uint32_t testfs_mkdir(struct fs_node *node, char *name) {
	struct file *new = kmalloc(sizeof(struct file));
	new->name = name;
	new->size = 0;
	new->allocated_size = sizeof(struct fs_node);
	new->uid = 0;
	new->gid = 0;
	new->lock = 0;
	new->read = NULL;
	new->write = NULL;
	new->readdir = testfs_readdir;
	new->type = S_IFMT &S_IFDIR;
	new->data = kmalloc(new->allocated_size);

	struct fs_node *folder = (struct fs_node *)new->data;

	vec_init(&folder->files);
	vec_init(&folder->nodes);

	folder->parent = node;
	folder->fs = node->fs;
	folder->target = kmalloc(256);
	strcat(folder->target, node->target);
	strcat(folder->target, name);
	strcat(folder->target, "/");
	vec_push(&node->files, new);
	vec_push(&node->nodes, folder);
	return 0;
}

struct fs testfs = {.name = "testfs",
					.open = testfs_open,
					.create = testfs_create,
					.delete = testfs_delete,
					.mkdir = testfs_mkdir};
