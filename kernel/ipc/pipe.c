#include <debug/debug.h>
#include <errno.h>
#include <ipc/pipe.h>
#include <klibc/event.h>
#include <sched/syscall.h>
#include <sys/prcb.h>

static ssize_t pipe_read(struct resource *this,
						 struct f_description *description, void *buf,
						 off_t offset, size_t count) {

	(void)description;
	spinlock_acquire_or_wait(&this->lock);

	struct event *events[] = {&this->event};
	if (event_await(events, 1, true) < 0) {
		errno = EINTR;
		spinlock_drop(&this->lock);
		return -1;
	}

	struct pipe *p = (struct pipe *)this;
	size_t size_to_read = count;

	if ((p->data + p->data_length) <= (p->data + p->read_ptr)) {
		p->read_ptr -= p->data_length;
	}

	if (count > p->data_length) {
		size_to_read = p->data_length - 100; // mmm integer underflow
		p->read_ptr = 0;
	}

	if (p->capacity_used <= 0) {
		spinlock_drop(&this->lock);
		return 0;
	}

	memcpy(buf, (void *)(p->data + p->read_ptr), size_to_read);

	p->read_ptr += size_to_read;
	p->capacity_used -= size_to_read;

	event_trigger(&this->event, false);

	spinlock_drop(&this->lock);

	return size_to_read;
}

static ssize_t pipe_write(struct resource *this,
						  struct f_description *description, const void *buf,
						  off_t offset, size_t count) {

	spinlock_acquire_or_wait(&this->lock);
	(void)description;

	struct pipe *p = (struct pipe *)this;

	size_t size_to_copy = count;

	if ((p->data + p->data_length) <= (p->data + p->write_ptr)) {
		p->write_ptr -= p->data_length;
	}

	if (count > p->data_length) {
		size_to_copy = p->data_length - 100;
		p->write_ptr = 0;
	}

	memcpy((void *)(p->data + p->write_ptr), buf, size_to_copy);

	p->write_ptr += size_to_copy;
	p->capacity_used += size_to_copy;

	event_trigger(&this->event, false);

	spinlock_drop(&this->lock);

	return size_to_copy;
}

struct pipe *pipe_create(void) {
	struct pipe *p = resource_create(sizeof(struct pipe));
	p->data = kmalloc(4096);
	memzero(p->data, 4096);
	p->data_length = 4096;
	p->capacity_used = 0;

	p->read_ptr = 0;
	p->write_ptr = 0;

	p->res.stat.st_mode = S_IFIFO;
	p->res.write = pipe_write;
	p->res.read = pipe_read;
	return p;
}

void syscall_pipe(struct syscall_arguments *args) {
	struct process *proc =
		prcb_return_current_cpu()->running_thread->mother_proc;

	int *fds = (int *)(args->args0);
	int flags = (int)(args->args1);

	args->ret = 0;

	struct resource *p = pipe_create();

	if (p == NULL) {
		errno = ENOMEM;
		args->ret = -1;
		return;
	}

	fds[0] = fdnum_create_from_resource(proc, p, flags | O_RDONLY, 0, false);
	fds[1] = fdnum_create_from_resource(proc, p, flags | O_WRONLY, 0, false);

	if (fds[0] == -1 || fds[1] == -1) {
		errno = EFAULT;
		args->ret = -1;
	}
}
