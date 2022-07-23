#include <debug/debug.h>
#include <fs/vfs.h>
#include <klibc/mem.h>
#include <locks/spinlock.h>

fs_node_vec_t nodes;
fs_vec_t filesystems;

bool is_init[2] = {false};

void vfs_install_fs(struct fs *fs) {
	if (!is_init[1]) {
		vec_init(&filesystems);
		is_init[1] = true;
	}
	vec_push(&filesystems, fs);
}

static struct fs *vfs_fs_name_to_fs(char *fs) {
	if (fs) {
		for (int i = 0; i < filesystems.length; i++) {
			if (!strcmp(filesystems.data[i]->name, fs))
				return filesystems.data[i];
		}
	}
	return NULL;
}

struct fs_node *vfs_node_create(struct fs_node *parent, char *name) {
	if (!is_init[0]) {
		vec_init(&nodes);
		is_init[0] = true;
	}
	struct fs_node *node = kmalloc(sizeof(struct fs_node));
	node->name = name;
	if (parent) {
		if (!parent->nodes.capacity)
			vec_init(&parent->nodes);
		vec_push(&parent->nodes, node);
		node->parent = parent;
	} else {
		node->parent = NULL;
	}
	vec_push(&nodes, node);
	return node;
}

bool vfs_node_mount(struct fs_node *node, char *target, char *fs) {
	for (int i = 0; i < nodes.length; i++) {
		if (nodes.data[i]->target) {
			if (!strcmp(nodes.data[i]->target, target)) {
				kprintf(
					"VFS: Failed to mount '%s' on '%s'. Mount already exists\n",
					node->name, node->target);
				return false;
			}
		}
	}
	struct fs *fs_ptr = vfs_fs_name_to_fs(fs);
	if (!fs_ptr) {
		kprintf("VFS: Failed to mount '%s' on '%s'. File system not found\n",
				node->name, node->target);
		return false;
	}
	node->fs = fs_ptr;
	node->target = target;
	vec_init(&node->files);
	kprintf("VFS: Mounted '%s' node on '%s' with file system '%s'\n", node->name,
			target, fs);
	return true;
}

void vfs_dump_fs_tree(struct fs_node *node) {
	kprintffos(0, "Dumping fs node\n");
	kprintffos(0, "%s\n", node->target);
	for (int i = 0; i < node->files.length; i++) {
		struct file *file = node->files.data[i];
		if (S_ISDIR(file->type)) {
			struct fs_node *nodea = file->readdir(file);
			vfs_dump_fs_tree(nodea);
		}
		else 
			kprintffos(0, "%s%s\n", node->target, file->name);
	}
}