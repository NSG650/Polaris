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

#include "vfs.h"
#include "../dev/dev.h"
#include "../klibc/lock.h"
#include "../klibc/printf.h"
#include "../klibc/string.h"
#include "../klibc/vec.h"
#include <liballoc.h>
#include <stdbool.h>
#include <stddef.h>

lock_t vfs_lock = {0};

typedef vec_t(struct filesystem *) filesystem_vec_t;
filesystem_vec_t filesystems;

bool initialized = false;

bool vfs_install_fs(struct filesystem *fs) {
	if (initialized == false) {
		vec_init(&filesystems);
		initialized = true;
	}
	vec_push(&filesystems, fs);
	return true;
}

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

static struct resource root_res = {
	.st = {.st_mode = S_IFDIR, .st_ino = VFS_ROOT_INODE}};

static struct vfs_node root_node = {.name = "/",
									.res = &root_res,
									.mount_data = NULL,
									.fs = NULL,
									.mount_gate = NULL,
									.parent = NULL,
									.child = NULL,
									.next = NULL,
									.backing_dev_id = 0};
enum { NO_CREATE = 0, CREATE_SHALLOW, CREATE_DEEP };

static struct vfs_node *path2node(struct vfs_node *parent, const char *_path,
								  int create) {
	bool last = false;

	if (_path == NULL)
		return NULL;

	if (*_path == 0)
		return NULL;

	char *path = kmalloc(strlen(_path) + 1);
	strcpy(path, _path);

	// Get rid of trailing slashes
	for (ssize_t i = strlen(path) - 1; i > 0; i--) {
		if (path[i] == '/')
			path[i] = 0;
		else
			break;
	}

	struct vfs_node *cur_parent =
		*path == '/' || parent == NULL ? &root_node : parent;
	if (cur_parent->mount_gate)
		cur_parent = cur_parent->mount_gate;
	struct vfs_node *cur_node = cur_parent->child;

	while (*path == '/') {
		path++;
		if (!*path)
			return &root_node;
	}

next:;
	char *elem = path;
	while (*path != 0) {
		if (*path == '/') {
			if (*(path + 1) == '/') {
				path++;
				continue;
			} else {
				break;
			}
		}
		path++;
	}
	if (*path == '/')
		*path++ = 0;
	else /* path == 0 */
		last = true;

	if (cur_node == NULL)
		goto epilogue;

	for (;;) {
		if (strcmp(cur_node->name, elem)) {
			if (cur_node->next == NULL)
				break;
			cur_node = cur_node->next;
			continue;
		}

		if (last) {
			return cur_node;
		}

		if (!S_ISDIR(cur_node->res->st.st_mode)) {
			// errno = ENOTDIR;
			return NULL;
		}

		if (cur_node->mount_gate != NULL)
			cur_node = cur_node->mount_gate;

		if (cur_node->child == NULL) {
			cur_node->child = cur_node->fs->populate(cur_node);
			if (cur_node->child == NULL) {
				// errno = ENOTDIR;
				return NULL;
			}
		}

		cur_parent = cur_node;
		cur_node = cur_node->child;
		goto next;
	}

epilogue:
	if (create) {
		if (last) {
			return vfs_new_node(cur_parent, elem);
		} else {
			if (create == CREATE_SHALLOW)
				return NULL;
			cur_parent = vfs_mkdir(cur_parent, elem, 0755, false);
			goto next;
		}
	}

	// if (last)
	//     errno = ENOENT;
	// else
	//     errno = ENOTDIR;
	return NULL;
}

static struct filesystem *fstype2fs(const char *fstype) {
	for (int i = 0; i < filesystems.length; i++) {
		if (!strcmp(filesystems.data[i]->name, fstype))
			return filesystems.data[i];
	}

	return NULL;
}

bool vfs_mount(const char *source, const char *target, const char *fstype) {
	struct filesystem *fs = fstype2fs(fstype);
	if (fs == NULL)
		return false;

	struct vfs_node *tgt_node = path2node(NULL, target, NO_CREATE);
	if (tgt_node == NULL)
		return false;

	if (!S_ISDIR(tgt_node->res->st.st_mode)) {
		// errno = ENOTDIR;
		return false;
	}

	dev_t backing_dev_id;
	struct resource *src_handle = NULL;
	if (fs->needs_backing_device) {
		struct vfs_node *backing_dev_node = path2node(NULL, source, NO_CREATE);
		if (backing_dev_node == NULL)
			return false;
		if (!S_ISCHR(backing_dev_node->res->st.st_mode) &&
			!S_ISBLK(backing_dev_node->res->st.st_mode))
			return false;
		struct resource *src_handle = vfs_open(source, O_RDWR, 0);
		if (src_handle == NULL)
			return false;
		backing_dev_id = backing_dev_node->res->st.st_rdev;
	} else {
		backing_dev_id = dev_new_id();
	}

	struct vfs_node *mount_gate = fs->mount(src_handle);
	if (mount_gate == NULL) {
		// vfs_close(src_handle);
		return false;
	}

	mount_gate->backing_dev_id = backing_dev_id;

	tgt_node->mount_gate = mount_gate;

	printf("VFS: Mounted '%s' on '%s', type: '%s'.\n", source, target, fstype);

	return true;
}

struct vfs_node *vfs_mkdir(struct vfs_node *parent, const char *name,
						   mode_t mode, bool recurse) {
	if (parent == NULL)
		parent = &root_node;

	struct vfs_node *new_dir = path2node(parent, name, NO_CREATE);

	if (new_dir != NULL)
		return NULL;

	new_dir = path2node(parent, name, recurse ? CREATE_DEEP : CREATE_SHALLOW);

	if (new_dir == NULL)
		return NULL;

	new_dir->res = new_dir->fs->mkdir(new_dir, mode);

	struct vfs_node *dot = vfs_new_node(new_dir, ".");
	dot->child = new_dir;
	dot->res = new_dir->res;

	struct vfs_node *dotdot = vfs_new_node(new_dir, "..");
	dotdot->child = parent;
	dotdot->res = parent->res;

	return new_dir;
}

struct vfs_node *vfs_new_node(struct vfs_node *parent, const char *name) {
	if (parent == NULL)
		parent = &root_node;

	if (parent->mount_gate)
		parent = parent->mount_gate;

	struct vfs_node *new_node = path2node(parent, name, NO_CREATE);

	if (new_node != NULL)
		return NULL;

	new_node = kmalloc(sizeof(struct vfs_node));

	new_node->next = parent->child;
	parent->child = new_node;

	strcpy(new_node->name, name);
	new_node->fs = parent->fs;
	new_node->mount_data = parent->mount_data;
	new_node->backing_dev_id = parent->backing_dev_id;
	new_node->parent = parent;

	return new_node;
}

struct vfs_node *vfs_new_node_deep(struct vfs_node *parent, const char *name) {
	struct vfs_node *new_node = path2node(parent, name, NO_CREATE);

	if (new_node != NULL)
		return NULL;

	new_node = path2node(parent, name, CREATE_DEEP);

	return new_node;
}

struct resource *vfs_open(const char *path, int oflags, mode_t mode) {
	LOCK(vfs_lock);

	bool create = oflags & O_CREAT;
	struct vfs_node *path_node =
		path2node(NULL, path, create ? CREATE_SHALLOW : NO_CREATE);
	if (path_node == NULL) {
		UNLOCK(vfs_lock);
		return NULL;
	}

	if (path_node->res == NULL)
		path_node->res = path_node->fs->open(path_node, create, mode);

	if (path_node->res == NULL) {
		UNLOCK(vfs_lock);
		return NULL;
	}

	struct resource *res = path_node->res;

	LOCK(res->lock);
	res->refcount++;
	UNLOCK(res->lock);

	UNLOCK(vfs_lock);

	return res;
}

void vfs_dump_nodes(struct vfs_node *node, const char *parent) {
	struct vfs_node *cur_node = node ? node : &root_node;
	for (; cur_node; cur_node = cur_node->next) {
		printf("%s - %s\n", parent, cur_node->name);
		if (!strcmp(cur_node->name, ".") || !strcmp(cur_node->name, ".."))
			continue;
		if (cur_node->mount_gate != NULL &&
			cur_node->mount_gate->child != NULL) {
			vfs_dump_nodes(cur_node->mount_gate->child, cur_node->name);
		} else if (cur_node->child != NULL && cur_node->mount_gate == NULL) {
			vfs_dump_nodes(cur_node->child, cur_node->name);
		}
	}
}

bool vfs_stat(const char *path, struct stat *st) {
	LOCK(vfs_lock);

	struct vfs_node *node = path2node(NULL, path, NO_CREATE);
	if (node == NULL) {
		// errno = ENOENT;
		UNLOCK(vfs_lock);
		return false;
	}

	*st = node->res->st;

	UNLOCK(vfs_lock);
	return true;
}
