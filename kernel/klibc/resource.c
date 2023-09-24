#include <debug/debug.h>
#include <errno.h>
#include <fs/vfs.h>
#include <klibc/resource.h>
#include <mm/slab.h>
#include <sched/sched.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/prcb.h>
#include <types.h>

int resource_default_ioctl(struct resource *this,
						   struct f_description *description, uint64_t request,
						   uint64_t arg) {
	(void)this;
	(void)description;
	(void)arg;

	switch (request) {
		case TCGETS:
		case TCSETS:
		case TIOCSCTTY:
		case TIOCGWINSZ:
			errno = ENOTTY;
			return -1;
	}

	errno = EINVAL;
	return -1;
}

static ssize_t stub_read(struct resource *this,
						 struct f_description *description, void *buf,
						 off_t offset, size_t count) {
	(void)this;
	(void)description;
	(void)buf;
	(void)offset;
	(void)count;
	errno = ENOSYS;
	return -1;
}

static ssize_t stub_write(struct resource *this,
						  struct f_description *description, const void *buf,
						  off_t offset, size_t count) {
	(void)this;
	(void)description;
	(void)buf;
	(void)offset;
	(void)count;
	errno = ENOSYS;
	return -1;
}

static void *stub_mmap(struct resource *this, size_t file_page, int flags) {
	(void)this;
	(void)file_page;
	(void)flags;
	return NULL;
}

static bool stub_unref(struct resource *this,
					   struct f_description *description) {
	(void)this;
	(void)description;
	this->refcount--;
	return true;
}

static bool stub_truncate(struct resource *this,
						  struct f_description *description, size_t length) {
	(void)this;
	(void)description;
	(void)length;
	errno = ENOSYS;
	return false;
}

void *resource_create(size_t size) {
	struct resource *res = kmalloc(size);
	if (res == NULL) {
		return NULL;
	}

	res->read = stub_read;
	res->write = stub_write;
	res->ioctl = resource_default_ioctl;
	res->mmap = stub_mmap;
	res->unref = stub_unref;
	res->truncate = stub_truncate;
	return res;
}

dev_t resource_create_dev_id(void) {
	static dev_t dev_id_counter = 1;
	static lock_t lock = {0};
	spinlock_acquire_or_wait(&lock);
	dev_t ret = dev_id_counter++;
	spinlock_drop(&lock);
	return ret;
}

bool fdnum_close(struct process *proc, int fdnum) {
	if (proc == NULL) {
		proc = prcb_return_current_cpu()->running_thread->mother_proc;
	}

	bool ok = false;
	spinlock_acquire_or_wait(&proc->fds_lock);

	if (fdnum < 0 || fdnum >= MAX_FDS) {
		errno = EBADF;
		goto cleanup;
	}

	struct f_descriptor *fd = proc->fds[fdnum];
	if (fd == NULL) {
		errno = EBADF;
		goto cleanup;
	}

	fd->description->res->unref(fd->description->res, fd->description);

	if (fd->description->refcount-- == 1) {
		kfree(fd->description);
	}

	kfree(fd);

	ok = true;
	proc->fds[fdnum] = NULL;

cleanup:
	spinlock_drop(&proc->fds_lock);
	return ok;
}

int fdnum_create_from_fd(struct process *proc, struct f_descriptor *fd,
						 int old_fdnum, bool specific) {
	if (proc == NULL) {
		proc = prcb_return_current_cpu()->running_thread->mother_proc;
	}

	int res = -1;
	spinlock_acquire_or_wait(&proc->fds_lock);

	if (old_fdnum < 0 || old_fdnum >= MAX_FDS) {
		errno = EBADF;
		goto cleanup;
	}

	if (!specific) {
		for (int i = old_fdnum; i < MAX_FDS; i++) {
			if (proc->fds[i] == NULL) {
				proc->fds[i] = fd;
				res = i;
				goto cleanup;
			}
		}
	} else {
		// TODO: Close an existing descriptor without deadlocking :^)
		// fdnum_close(proc, old_fdnum);
		proc->fds[old_fdnum] = fd;
		res = old_fdnum;
	}

cleanup:
	spinlock_drop(&proc->fds_lock);
	return res;
}

int fdnum_create_from_resource(struct process *proc, struct resource *res,
							   int flags, int old_fdnum, bool specific) {
	struct f_descriptor *fd = fd_create_from_resource(res, flags);
	if (fd == NULL) {
		return -1;
	}

	return fdnum_create_from_fd(proc, fd, old_fdnum, specific);
}

int fdnum_dup(struct process *old_proc, int old_fdnum, struct process *new_proc,
			  int new_fdnum, int flags, bool specific, bool cloexec) {
	if (old_proc == NULL) {
		old_proc = prcb_return_current_cpu()->running_thread->mother_proc;
	}

	if (new_proc == NULL) {
		new_proc = prcb_return_current_cpu()->running_thread->mother_proc;
	}

	if (specific && old_fdnum == new_fdnum && old_proc == new_proc) {
		errno = EINVAL;
		return -1;
	}

	struct f_descriptor *old_fd = fd_from_fdnum(old_proc, old_fdnum);
	if (old_fd == NULL) {
		return -1;
	}

	struct f_descriptor *new_fd = kmalloc(sizeof(struct f_descriptor));
	if (new_fd == NULL) {
		errno = ENOMEM;
		return -1;
	}

	memcpy(new_fd, old_fd, sizeof(struct f_descriptor));

	new_fdnum = fdnum_create_from_fd(new_proc, new_fd, new_fdnum, specific);
	if (new_fdnum < 0) {
		kfree(new_fd);
		return -1;
	}

	new_fd->flags = flags & FILE_DESCRIPTOR_FLAGS_MASK;
	if (cloexec) {
		new_fd->flags &= O_CLOEXEC;
	}

	old_fd->description->refcount++;
	old_fd->description->res->refcount++;

	return new_fdnum;
}

struct f_descriptor *fd_create_from_resource(struct resource *res, int flags) {
	res->refcount++;

	struct f_description *description = kmalloc(sizeof(struct f_description));
	if (description == NULL) {
		goto fail;
	}

	memzero(description, sizeof(struct f_description));

	description->refcount = 1;
	description->flags = flags & FILE_STATUS_FLAGS_MASK;
	spinlock_init(description->lock);
	description->res = res;

	struct f_descriptor *fd = kmalloc(sizeof(struct f_descriptor));
	if (fd == NULL) {
		goto fail;
	}

	fd->description = description;
	fd->flags = flags & FILE_DESCRIPTOR_FLAGS_MASK;
	return fd;

fail:
	res->refcount--;
	if (description != NULL) {
		kfree(description);
	}
	return NULL;
}

struct f_descriptor *fd_from_fdnum(struct process *proc, int fdnum) {
	if (proc == NULL) {
		proc = prcb_return_current_cpu()->running_thread->mother_proc;
	}

	struct f_descriptor *ret = NULL;
	spinlock_acquire_or_wait(&proc->fds_lock);

	if (fdnum < 0 || fdnum >= MAX_FDS) {
		errno = EBADF;
		goto cleanup;
	}

	ret = proc->fds[fdnum];
	if (ret == NULL) {
		errno = EBADF;
		goto cleanup;
	}

	ret->description->refcount++;

cleanup:
	spinlock_drop(&proc->fds_lock);
	return ret;
}

void syscall_close(struct syscall_arguments *args) {
	struct process *proc =
		prcb_return_current_cpu()->running_thread->mother_proc;

	args->ret = fdnum_close(proc, args->args0) ? 0 : -1;
}

void syscall_read(struct syscall_arguments *args) {
	struct process *proc =
		prcb_return_current_cpu()->running_thread->mother_proc;

	int fdnum = args->args0;
	void *buf = (void *)args->args1;
	size_t count = args->args2;

	struct f_descriptor *fd = fd_from_fdnum(proc, fdnum);
	if (fd == NULL) {
		args->ret = -1;
		return;
	}

	struct f_description *description = fd->description;
	struct resource *res = description->res;

	ssize_t read = res->read(res, description, buf, description->offset, count);
	if (read < 0) {
		args->ret = -1;
		return;
	}

	description->offset += read;
	args->ret = read;
}

void syscall_write(struct syscall_arguments *args) {
	struct process *proc =
		prcb_return_current_cpu()->running_thread->mother_proc;

	int fdnum = args->args0;
	void *buf = (void *)args->args1;
	size_t count = args->args2;

	struct f_descriptor *fd = fd_from_fdnum(proc, fdnum);
	if (fd == NULL) {
		args->ret = -1;
		return;
	}

	struct f_description *description = fd->description;
	struct resource *res = description->res;

	ssize_t written =
		res->write(res, description, buf, description->offset, count);
	if (written < 0) {
		args->ret = -1;
		return;
	}

	description->offset += written;
	args->ret = written;
}

void syscall_seek(struct syscall_arguments *args) {
	struct process *proc =
		prcb_return_current_cpu()->running_thread->mother_proc;

	int fdnum = args->args0;
	off_t offset = args->args1;
	int whence = args->args2;

	struct f_descriptor *fd = fd_from_fdnum(proc, fdnum);
	if (fd == NULL) {
		args->ret = -1;
		return;
	}

	struct f_description *description = fd->description;
	switch (description->res->stat.st_mode & S_IFMT) {
		case S_IFCHR:
		case S_IFIFO:
		case S_IFSOCK:
			errno = ESPIPE;
			args->ret = -1;
			return;
	}

	off_t curr_offset = description->offset;
	off_t new_offset = 0;

	switch (whence) {
		case SEEK_CUR:
			new_offset = curr_offset + offset;
			break;
		case SEEK_END:
			new_offset = offset + description->res->stat.st_size;
			break;
		case SEEK_SET:
			new_offset = offset;
			break;
		default:
			errno = EINVAL;
			args->ret = -1;
			return;
	}

	if (new_offset < 0) {
		errno = EINVAL;
		args->ret = -1;
		return;
	}

	// TODO: Implement res->grow
	// if (new_offset >= fd->description->res->stat.st_size) {
	//     description->res->grow(description->res, new_offset);
	// }

	description->offset = new_offset;
	args->ret = new_offset;
}

void syscall_fcntl(struct syscall_arguments *args) {
	struct process *proc =
		prcb_return_current_cpu()->running_thread->mother_proc;

	int fdnum = args->args0;
	uint64_t request = args->args1;
	uint64_t arg = args->args2;

	struct f_descriptor *fd = fd_from_fdnum(proc, fdnum);
	if (fd == NULL) {
		args->ret = -1;
		return;
	}

	switch (request) {
		case F_DUPFD:
			args->ret = fdnum_dup(proc, fdnum, proc, (int)arg, 0, false, false);
			return;
		case F_DUPFD_CLOEXEC:
			args->ret = fdnum_dup(proc, fdnum, proc, (int)arg, 0, false, true);
			return;
		case F_GETFD:
			if ((fd->flags & O_CLOEXEC) != 0) {
				args->ret = O_CLOEXEC;
				return;
			} else {
				args->ret = 0;
				return;
			}
		case F_SETFD:
			if ((arg & O_CLOEXEC) != 0) {
				fd->flags = O_CLOEXEC;
			} else {
				fd->flags = 0;
			}
			args->ret = 0;
			return;
		case F_GETFL:
			args->ret = fd->description->flags;
			return;
		case F_SETFL:
			fd->description->flags = (int)arg;
			args->ret = 0;
			return;
		default:
			errno = EINVAL;
			args->ret = -1;
			return;
	}
}

void syscall_ioctl(struct syscall_arguments *args) {
	struct process *proc =
		prcb_return_current_cpu()->running_thread->mother_proc;

	int fdnum = args->args0;
	uint64_t request = args->args1;
	uint64_t arg = args->args2;

	struct f_descriptor *fd = fd_from_fdnum(proc, fdnum);
	if (fd == NULL) {
		args->ret = -1;
		return;
	}

	struct f_description *description = fd->description;
	struct resource *res = description->res;
	args->ret = res->ioctl(res, description, request, arg);
}

void syscall_dup3(struct syscall_arguments *args) {
	struct process *proc =
		prcb_return_current_cpu()->running_thread->mother_proc;

	int old_fdnum = args->args0;
	int new_fdnum = args->args1;
	int flags = args->args2;

	args->ret = fdnum_dup(proc, old_fdnum, proc, new_fdnum, flags, true, false);
}

void syscall_fchmodat(struct syscall_arguments *args) {
	int dir_fdnum = args->args0;
	const char *path = (char *)args->args1;
	mode_t mode = args->args2;

	struct vfs_node *parent = NULL, *node = NULL;
	if (!vfs_fdnum_path_to_node(dir_fdnum, path, true, true, &parent, &node,
								NULL)) {
		args->ret = -1;
		return;
	}

	struct vfs_node *target = node;
	if (target == NULL) {
		target = parent;
	}

	target->resource->stat.st_mode &= ~0777;
	target->resource->stat.st_mode |= mode & 0777;
	args->ret = 0;
}
