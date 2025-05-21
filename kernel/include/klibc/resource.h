#ifndef RESOURCE_H
#define RESOURCE_H

#include <klibc/event.h>
#include <locks/spinlock.h>
#include <sched/syscall.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

struct process;
struct f_description;

struct resource {
	int status;
	struct event event;
	size_t refcount;
	lock_t lock;
	struct stat stat;
	bool can_mmap;

	ssize_t (*read)(struct resource *this, struct f_description *description,
					void *buf, off_t offset, size_t count);
	ssize_t (*write)(struct resource *this, struct f_description *description,
					 const void *buf, off_t offset, size_t count);
	int (*ioctl)(struct resource *this, struct f_description *description,
				 uint64_t request, uint64_t arg);
	void *(*mmap)(struct resource *this, size_t file_page, int flags);
	bool (*ref)(struct resource *this, struct f_description *description);
	bool (*unref)(struct resource *this, struct f_description *description);
	bool (*truncate)(struct resource *this, struct f_description *description,
					 size_t length);
};

struct f_description {
	size_t refcount;
	off_t offset;
	bool is_dir;
	int flags;
	lock_t lock;
	struct resource *res;
	struct vfs_node *node;
};

struct f_descriptor {
	struct f_description *description;
	int flags;
};

struct pollfd {
	int fd;
	short events;
	short revents;
};

void *resource_create(size_t size);
dev_t resource_create_dev_id(void);

bool fdnum_close(struct process *proc, int fdnum);
int fdnum_create_from_fd(struct process *proc, struct f_descriptor *fd,
						 int old_fdnum, bool specific);
int fdnum_create_from_resource(struct process *proc, struct resource *res,
							   int flags, int old_fdnum, bool specific);
int fdnum_dup(struct process *old_proc, int old_fdnum, struct process *new_proc,
			  int new_fdnum, int flags, bool specific, bool cloexec);
struct f_descriptor *fd_create_from_resource(struct resource *res, int flags);
struct f_descriptor *fd_from_fdnum(struct process *proc, int fdnum);

int resource_default_ioctl(struct resource *this,
						   struct f_description *description, uint64_t request,
						   uint64_t arg);

#define FILE_CREATION_FLAGS_MASK \
	(O_CREAT | O_DIRECTORY | O_EXCL | O_NOCTTY | O_NOFOLLOW | O_TRUNC)
#define FILE_DESCRIPTOR_FLAGS_MASK (O_CLOEXEC)
#define FILE_STATUS_FLAGS_MASK \
	(~(FILE_CREATION_FLAGS_MASK | FILE_DESCRIPTOR_FLAGS_MASK))

void syscall_close(struct syscall_arguments *args);
void syscall_read(struct syscall_arguments *args);
void syscall_write(struct syscall_arguments *args);
void syscall_seek(struct syscall_arguments *args);
void syscall_fcntl(struct syscall_arguments *args);
void syscall_ioctl(struct syscall_arguments *args);
void syscall_dup3(struct syscall_arguments *args);
void syscall_fchmodat(struct syscall_arguments *args);
void syscall_ppoll(struct syscall_arguments *args);

#endif
