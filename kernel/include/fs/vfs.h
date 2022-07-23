#ifndef VFS_H
#define VFS_H

#include <klibc/vec.h>
#include <locks/spinlock.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

struct fs;
struct fs_node;
struct file;

typedef vec_t(struct fs_node *) fs_node_vec_t;
typedef vec_t(struct fs *) fs_vec_t;
typedef vec_t(struct file *) file_vec_t;

struct fs_node {
	char *name;
	char *target;
	struct fs *fs;
	file_vec_t files;
	struct fs_node *parent;
	fs_node_vec_t nodes;
};

struct fs {
	char *name;
	struct file *(*open)(struct fs_node *, char *);
	uint32_t (*create)(struct fs_node *, char *);
	uint32_t (*delete)(struct fs_node *, char *);
	uint32_t (*mkdir)(struct fs_node *, char *);
};

struct file {
	char *name;
	lock_t lock;
	int32_t uid;
	int32_t gid;
	int32_t type;
	size_t size;
	size_t allocated_size;
	uint32_t (*read)(struct file *, size_t, size_t, uint8_t *);
	uint32_t (*write)(struct file *, size_t, size_t, uint8_t *);
	struct fs_node *(*readdir)(struct file *);
	uint8_t *data;
};

void vfs_install_fs(struct fs *fs);
struct fs_node *vfs_node_create(struct fs_node *parent, char *name);
bool vfs_node_mount(struct fs_node *node, char *target, char *fs);

#endif
