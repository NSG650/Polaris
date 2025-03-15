#include <debug/debug.h>
#include <errno.h>
#include <ipc/pipe.h>
#include <klibc/event.h>
#include <sched/syscall.h>
#include <sys/prcb.h>

static bool pipe_unref(struct resource *this,
					   struct f_description *description) {
	(void)description;
	event_trigger(&this->event, false);
	this->refcount--;
	struct pipe *p = (struct pipe *)this;
	if (!this->refcount) {
		kfree(p->data);
	}
	return true;
}

static ssize_t pipe_read(struct resource *this,
						 struct f_description *description, void *buf,
						 off_t offset, size_t count) {

	(void)description;
	(void)offset;
	struct pipe *p = (struct pipe *)this;
	uint8_t *d = buf;

	if (p->read_ptr == p->write_ptr) {
		event_trigger(&this->event, false);
		return 0;
	}

	spinlock_acquire_or_wait(&this->lock);

	size_t i = 0;
	for (i = 0; i < count; i++) {
		if (p->write_ptr == p->read_ptr) {
			this->status &= ~POLLIN;
			break;
		}
		d[i] = p->data[p->read_ptr++ % p->data_length];
	}

	if (p->read_ptr != p->write_ptr) {
		this->status |= POLLOUT;
	}

	event_trigger(&this->event, false);
	spinlock_drop(&this->lock);
	return i;
}

static ssize_t pipe_write(struct resource *this,
						  struct f_description *description, const void *buf,
						  off_t offset, size_t count) {

	spinlock_acquire_or_wait(&this->lock);
	(void)description;
	(void)offset;

	struct pipe *p = (struct pipe *)this;
	const uint8_t *d = buf;

	for (size_t i = 0; i < count; i++) {
		while (p->write_ptr == p->read_ptr + p->data_length) {
			event_trigger(&this->event, false);
			struct event *events[] = {&this->event};
			spinlock_drop(&this->lock);
			if (event_await(events, 1, true) < 0) {
				errno = EINTR;
				return -1;
			}
			spinlock_acquire_or_wait(&this->lock);
		}
		p->data[p->write_ptr++ % p->data_length] = d[i];
	}

	if (p->write_ptr == p->read_ptr + p->data_length) {
		this->status &= ~POLLOUT;
	}

	this->status |= POLLIN;

	event_trigger(&this->event, false);
	spinlock_drop(&this->lock);
	return count;
}

struct pipe *pipe_create(void) {
	struct pipe *p = resource_create(sizeof(struct pipe));
	p->data = kmalloc(PAGE_SIZE * 32);
	memzero(p->data, PAGE_SIZE * 32);
	p->data_length = PAGE_SIZE * 32;

	p->read_ptr = 0;
	p->write_ptr = 0;

	p->res.stat.st_mode = S_IFIFO;
	p->res.write = pipe_write;
	p->res.read = pipe_read;
	p->res.unref = pipe_unref;
	return p;
}

void syscall_pipe(struct syscall_arguments *args) {
	struct process *proc = sched_get_running_thread()->mother_proc;

	int *fds = (int *)(args->args0);
	int flags = (int)(args->args1);

	args->ret = 0;

	if (fds == NULL) {
		errno = EFAULT;
		args->ret = -1;
		return;
	}

	struct resource *p = (struct resource *)pipe_create();

	if (p == NULL) {
		errno = ENOMEM;
		args->ret = -1;
		return;
	}

	fds[0] = fdnum_create_from_resource(proc, p, flags | O_RDONLY, 0, false);
	fds[1] = fdnum_create_from_resource(proc, p, flags | O_WRONLY, 0, false);

	if (fds[0] == -1 || fds[1] == -1) {
		args->ret = -1;
	}
}
