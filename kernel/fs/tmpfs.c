#include <fs/vfs.h>
#include <klibc/mem.h>

struct tmpfs_private_data {
	uint8_t *actual_data;
	size_t allocated_size;
};

struct fs_node *tmpfs_readdir(struct file *file) {
	return (struct fs_node *)file->data;
}

struct stat *tmpfs_fstat(struct file *file) {
	return &file->fstat;
}

ssize_t tmpfs_read(struct file *this, void *buf, off_t offset, size_t count) {
	spinlock_acquire_or_wait(this->lock);
	struct tmpfs_private_data *tpd = (struct tmpfs_private_data *)this->data;
	size_t actual_count = count;

	if ((off_t)(offset + count) >= (off_t)this->fstat.st_size)
		actual_count = count - ((offset + count) - this->fstat.st_size);

	memcpy(buf, tpd->actual_data + offset, actual_count);
	spinlock_drop(this->lock);

	return actual_count;
}

ssize_t tmpfs_write(struct file *this, const void *buf, off_t offset,
					size_t count) {
	spinlock_acquire_or_wait(this->lock);
	struct tmpfs_private_data *tpd = (struct tmpfs_private_data *)this->data;
	if (offset + count >= tpd->allocated_size) {
		size_t new_capacity = tpd->allocated_size;
		while (offset + count >= new_capacity)
			new_capacity *= 2;

		tpd->actual_data = krealloc(tpd->actual_data, new_capacity);
		tpd->allocated_size = new_capacity;
	}

	memcpy(tpd->actual_data + offset, buf, count);
	this->fstat.st_size += (offset + count) - this->fstat.st_size;

	spinlock_drop(this->lock);

	return (ssize_t)count;
}

struct file *tmpfs_open(struct fs_node *node, char *name) {
	for (int i = 0; i < node->files.length; i++) {
		if (!strcmp(node->files.data[i]->name, name))
			return node->files.data[i];
	}
	return NULL;
}

uint32_t tmpfs_delete(struct fs_node *node, char *name) {
	struct file *filex = NULL;
	for (int i = 0; i < node->files.length; i++) {
		if (!strcmp(node->files.data[i]->name, name)) {
			filex = node->files.data[i];
			break;
		}
	}
	// If you can't write. You can't delete them either
	if (filex && filex->write) {
		vec_remove(&node->files, filex);
		kfree(filex->data);
		kfree(filex);
		return 0;
	} else
		return 1;
}

uint32_t tmpfs_create(struct fs_node *node, char *name) {
	for (int i = 0; i < node->files.length; i++) {
		if (!strcmp(node->files.data[i]->name, name)) {
			return 1;
		}
	}
	struct file *new = kmalloc(sizeof(struct file));
	new->name = name;
	new->lock = 0;
	new->read = tmpfs_read;
	new->write = tmpfs_write;
	new->readdir = NULL;
	new->stat = tmpfs_fstat;
	new->data = kmalloc(sizeof(struct tmpfs_private_data));

	struct tmpfs_private_data *tpd = (struct tmpfs_private_data *)new->data;
	tpd->allocated_size = 1024;
	tpd->actual_data = kmalloc(tpd->allocated_size);

	vec_push(&node->files, new);
	return 0;
}

uint32_t tmpfs_mkdir(struct fs_node *node, char *name) {
		for (int i = 0; i < node->files.length; i++) {
		if (!strcmp(node->files.data[i]->name, name)) {
			return 1;
		}
	}
	struct file *new = kmalloc(sizeof(struct file));
	new->name = name;
	new->lock = 0;
	new->read = NULL;
	new->write = NULL;
	new->stat = tmpfs_fstat;
	new->readdir = tmpfs_readdir;
	new->data = kmalloc(sizeof(struct fs_node));

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

struct fs tmpfs = {.name = "tmpfs",
				   .open = tmpfs_open,
				   .create = tmpfs_create,
				   .delete = tmpfs_delete,
				   .mkdir = tmpfs_mkdir};
