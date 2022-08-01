#include <debug/debug.h>
#include <errno.h>
#include <fs/vfs.h>
#include <klibc/mem.h>
#include <locks/spinlock.h>
#include <sched/sched.h>
#include <sys/prcb.h>

fs_node_vec_t mount_nodes;
fs_vec_t filesystems;

bool is_init[2] = {false};

// Original Author for this function is mintsuki (https://github.com/mintsuki)

/* Convert a relative path into an absolute path.
   This is a freestanding function and can be used for any purpose :) */
void vfs_get_absolute_path(char *path_ptr, const char *path, const char *pwd) {
	char *orig_ptr = path_ptr;

	if (!*path) {
		strcpy(path_ptr, pwd);
		return;
	}

	if (*path != '/') {
		strcpy(path_ptr, pwd);
		path_ptr += strlen(path_ptr);
	} else {
		*path_ptr = '/';
		path_ptr++;
		path++;
	}

	goto first_run;

	for (;;) {
		switch (*path) {
			case '/':
				path++;
			first_run:
				if (*path == '/')
					continue;
				if ((!strncmp(path, ".\0", 2)) || (!strncmp(path, "./\0", 3))) {
					goto term;
				}
				if ((!strncmp(path, "..\0", 3)) ||
					(!strncmp(path, "../\0", 4))) {
					while (*path_ptr != '/')
						path_ptr--;
					if (path_ptr == orig_ptr)
						path_ptr++;
					goto term;
				}
				if (!strncmp(path, "../", 3)) {
					while (*path_ptr != '/')
						path_ptr--;
					if (path_ptr == orig_ptr)
						path_ptr++;
					path += 2;
					*path_ptr = 0;
					continue;
				}
				if (!strncmp(path, "./", 2)) {
					path += 1;
					continue;
				}
				if (((path_ptr - 1) != orig_ptr) && (*(path_ptr - 1) != '/')) {
					*path_ptr = '/';
					path_ptr++;
				}
				continue;
			case '\0':
			term:
				if ((*(path_ptr - 1) == '/') && ((path_ptr - 1) != orig_ptr))
					path_ptr--;
				*path_ptr = 0;
				return;
			default:
				*path_ptr = *path;
				path++;
				path_ptr++;
				continue;
		}
	}
}

static struct fs_node *vfs_look_for_node_under_node(struct fs_node *node,
													char **name, size_t idx,
													size_t max_idx) {
	for (int i = 0; i < node->files.length; i++) {
		struct file *a = node->files.data[i];
		if (!strcmp(a->name, name[idx])) {
			if (S_ISDIR(a->type)) {
				struct fs_node *nodea = a->readdir(a);
				if (idx != max_idx) {
					return vfs_look_for_node_under_node(nodea, name, idx + 1,
														max_idx);
				} else {
					return nodea;
				}
			} else {
				return NULL;
			}
		}
	}
	return NULL;
}

// returns the node in which the file is present
struct fs_node *vfs_path_to_node(char *patha) {
	// split the string and store it in an array
	char **token_list = NULL;
	size_t token_count = strsplit(patha, '/', &token_list);

	if (token_count < 3) {
		// Its the root node
		kfree(token_list);
		return mount_nodes.data[0];
	}

	// look through every mounted node
	char *target_path = kmalloc(256);
	// last entry is the file
	for (size_t i = 1; i < token_count - 1; i++) {
		strcat(target_path, "/");
		strcat(target_path, token_list[i]);
		for (int j = 0; j < mount_nodes.length; j++) {
			if (!strcmp(mount_nodes.data[j]->target, target_path)) {
				kfree(token_list);
				kfree(target_path);
				return mount_nodes.data[j];
			}
		}
	}

	// look for the folder under the root node
	struct fs_node *da_node = vfs_look_for_node_under_node(
		mount_nodes.data[0], token_list, 1, token_count - 2);

	kfree(token_list);

	return da_node;
}

struct file *vfs_find_file_in_node(struct fs_node *node, char *path) {
	// just copy the string till '/' then reverse it
	// there you go
	// your file name

	char *file_name = kmalloc(strlen(path) + 1);
	size_t a = strlen(path) - 1;
	size_t c = 0;
	while (path[a] != '/') {
		file_name[c] = path[a];
		a--;
		c++;
	}
	file_name[c] = '\0';

	strrev(file_name);

	struct file *f = NULL;

	for (int i = 0; i < node->files.length; i++) {
		if (!strcmp(node->files.data[i]->name, file_name)) {
			f = node->files.data[i];
			break;
		}
	}

	kfree(file_name);
	return f;
}

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
		vec_init(&mount_nodes);
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
	vec_push(&mount_nodes, node);
	return node;
}

bool vfs_node_mount(struct fs_node *node, char *target, char *fs) {
	for (int i = 0; i < mount_nodes.length; i++) {
		if (mount_nodes.data[i]->target) {
			if (!strcmp(mount_nodes.data[i]->target, target)) {
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
	kprintf("VFS: Mounted '%s' node on '%s' with file system '%s'\n",
			node->name, target, fs);
	return true;
}

void vfs_dump_fs_tree(struct fs_node *node) {
	kprintffos(0, "%s\n", node->target);
	for (int i = 0; i < node->files.length; i++) {
		struct file *file = node->files.data[i];
		if (S_ISDIR(file->type)) {
			struct fs_node *nodea = file->readdir(file);
			vfs_dump_fs_tree(nodea);
		} else
			kprintffos(0, "%s%s\n", node->target, file->name);
	}
}

void syscall_open(struct syscall_arguments *args) {
	char *path = kmalloc(256);
	struct process *proc =
		prcb_return_current_cpu()->running_thread->mother_proc;
	vfs_get_absolute_path(path, (char *)args->args0, proc->cwd);
	struct file *file = vfs_open_file(path);
	if (file) {
		vec_push(&proc->file_descriptors, file);
		args->ret = proc->file_descriptors.length - 1;
	} else
		args->ret = -ENOENT;
	kfree(path);
}

void syscall_read(struct syscall_arguments *args) {
	struct process *proc =
		prcb_return_current_cpu()->running_thread->mother_proc;
	struct file *file = proc->file_descriptors.data[args->args0];
	uint8_t *data = kmalloc(args->args2);
	file->read(file, args->args2, 0, data);
	syscall_helper_copy_to_user(args->args1, data, args->args2);
	kfree(data);
}