#include <debug/debug.h>
#include <errno.h>
#include <ipc/pipe.h>
#include <klibc/event.h>
#include <sched/syscall.h>
#include <sys/prcb.h>

static bool pipe_unref(struct resource *this,
					   struct f_description *description) {
	(void)description;
	this->refcount--;
	event_trigger(&this->event, false);
	return true;
}

static ssize_t pipe_read(struct resource *this,
						 struct f_description *description, void *buf,
						 off_t offset, size_t count) {

	(void)description;
	(void)offset;
	struct pipe *p = (struct pipe *)this;
	size_t size_to_read = count;

	spinlock_acquire_or_wait(&this->lock);

	while (p->capacity_used == 0) {
		if (this->refcount <= 1) {
			spinlock_drop(&this->lock);
			return 0;
		}
		if ((description->flags & O_NONBLOCK) != 0) {
			spinlock_drop(&this->lock);
			return 0;
		}
		spinlock_drop(&this->lock);
		struct event *events[] = {&this->event};
		if (event_await(events, 1, true) < 0) {
			errno = EINTR;
			spinlock_drop(&this->lock);
			return -1;
		}
		spinlock_acquire_or_wait(&this->lock);
	}

	if (p->capacity_used < count) {
		count = p->capacity_used;
	}

	size_t before_wrap = 0, after_wrap = 0, new_ptr = 0;
	if (p->read_ptr + count > p->data_length) {
		before_wrap = p->data_length - p->read_ptr;
		after_wrap = count - before_wrap;
		new_ptr = after_wrap;
	} else {
		before_wrap = count;
		after_wrap = 0;
		new_ptr = p->read_ptr + count;

		if (new_ptr == p->data_length) {
			new_ptr = 0;
		}
	}

	memcpy(buf, p->data + p->read_ptr, before_wrap);
	if (after_wrap != 0) {
		memcpy(buf + before_wrap, p->data, after_wrap);
	}

	p->read_ptr = new_ptr;
	p->capacity_used -= count;

	if (p->capacity_used == 0) {
		this->status &= ~POLLIN;
	}
	if (p->capacity_used < p->data_length) {
		this->status |= POLLOUT;
	}

	event_trigger(&this->event, false);

	spinlock_drop(&this->lock);

	return size_to_read;
}

static ssize_t pipe_write(struct resource *this,
						  struct f_description *description, const void *buf,
						  off_t offset, size_t count) {

	spinlock_acquire_or_wait(&this->lock);
	(void)description;
	(void)offset;
	struct pipe *p = (struct pipe *)this;

	if (this->refcount < 2) {
		errno = EPIPE;
		spinlock_drop(&this->lock);
		return -1;
	}

	while (p->capacity_used == p->data_length) {
		spinlock_drop(&this->lock);
		struct event *events[] = {&this->event};
		if (event_await(events, 1, true) < 0) {
			errno = EINTR;
			spinlock_drop(&this->lock);
			return -1;
		}
		spinlock_acquire_or_wait(&this->lock);
	}

	if (p->capacity_used + count > p->data_length) {
		count = p->data_length - p->capacity_used;
	}

	size_t before_wrap = 0, after_wrap = 0, new_ptr = 0;
	if (p->write_ptr + count > p->data_length) {
		before_wrap = p->data_length - p->write_ptr;
		after_wrap = count - before_wrap;
		new_ptr = after_wrap;
	} else {
		before_wrap = count;
		after_wrap = 0;
		new_ptr = p->write_ptr + count;

		if (new_ptr == p->data_length) {
			new_ptr = 0;
		}
	}

	memcpy(p->data + p->write_ptr, buf, before_wrap);
	if (after_wrap != 0) {
		memcpy(p->data, buf + before_wrap, after_wrap);
	}

	p->write_ptr = new_ptr;
	p->capacity_used += count;

	if (p->capacity_used == p->data_length) {
		this->status &= POLLOUT;
	}

	this->status |= POLLIN;

	event_trigger(&this->event, false);

	spinlock_drop(&this->lock);

	return count;
}

struct pipe *pipe_create(void) {
	struct pipe *p = resource_create(sizeof(struct pipe));
	if (p == NULL) {
		return NULL;
	}
	p->data = kmalloc(65536);
	if (p->data == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	memzero(p->data, 65536);
	p->data_length = 65536;
	p->capacity_used = 0;

	p->read_ptr = 0;
	p->write_ptr = 0;

	p->res.stat.st_mode = S_IFIFO;
	p->res.write = pipe_write;
	p->res.read = pipe_read;
	p->res.unref = pipe_unref;

	return p;
}

void syscall_pipe(struct syscall_arguments *args) {
	struct process *proc =
		prcb_return_current_cpu()->running_thread->mother_proc;

	int *fds = (int *)(args->args0);
	int flags = (int)(args->args1);

	args->ret = 0;

	struct resource *p = (struct resource *)pipe_create();

	if (p == NULL) {
		args->ret = -1;
		return;
	}

	fds[0] = fdnum_create_from_resource(proc, p, flags | O_RDONLY, 0, false);
	fds[1] = fdnum_create_from_resource(proc, p, flags | O_WRONLY, 0, false);

	if (fds[0] == -1 || fds[1] == -1) {
		args->ret = -1;
	}
}
