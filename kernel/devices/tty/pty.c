#include <debug/debug.h>
#include <devices/tty/pty.h>
#include <errno.h>

// I need to think this through once.
// Does not work for now.
// Only outputs inputs don't work.

static int pty_number = 0;

static int pty_ioctl(struct resource *this, struct f_description *description,
					 uint64_t request, uint64_t arg) {
	(void)description;
	spinlock_acquire_or_wait(&this->lock);

	struct pty_slave *ps = (struct pty_slave *)this;
	struct pty *p = ps->pty;

	int ret = 0;

	switch (request) {
		case TCGETS: {
			struct termios *t = (void *)arg;
			if (t)
				*t = p->term;
			break;
		}
		case TCSETS:
		case TCSETSW:
		case TCSETSF: {
			struct termios *t = (void *)arg;
			if (t)
				p->term = *t;
			break;
		}
		case TIOCGWINSZ: {
			struct winsize *w = (void *)arg;
			if (w)
				*w = p->ws;
			break;
		}
		case TIOCSPGRP: {
			ret = 0;
			break;
		}
		case TIOCSWINSZ: {
			struct winsize *w = (void *)arg;
			if (w)
				p->ws = *w;
			break;
		}
		default:
			errno = EINVAL;
			ret = -1;
			break;
	}

	spinlock_drop(&this->lock);
	return ret;
}

static bool pty_master_unref(struct resource *this,
							 struct f_description *description) {
	(void)description;
	struct pty_master *pm = (struct pty_master *)this;
	this->refcount--;
	if (!this->refcount) {
		struct pty *p = pm->pty;
		kfree(p->in.data);
		kfree(p->out.data);
	}
	return true;
}

static ssize_t pty_master_read(struct resource *this,
							   struct f_description *description, void *buf,
							   off_t offset, size_t count) {

	(void)description;
	(void)offset;
	struct pty_master *pm = (struct pty_master *)this;
	struct pty *p = pm->pty;
	uint8_t *d = buf;

	if (p->in.read_ptr == p->in.write_ptr) {
		event_trigger(&p->in.ev, false);
		return 0;
	}

	spinlock_acquire_or_wait(&this->lock);

	size_t i = 0;
	for (i = 0; i < count; i++) {
		if (p->in.write_ptr == p->in.read_ptr) {
			this->status &= ~POLLIN;
			break;
		}
		d[i] = p->in.data[p->in.read_ptr++ % p->in.data_length];
	}

	if (p->in.read_ptr != p->in.write_ptr) {
		this->status |= POLLOUT;
	}

	event_trigger(&p->in.ev, false);
	spinlock_drop(&this->lock);
	return i;
}

static ssize_t pty_master_write(struct resource *this,
								struct f_description *description,
								const void *buf, off_t offset, size_t count) {

	spinlock_acquire_or_wait(&this->lock);
	(void)description;
	(void)offset;

	struct pty_master *pm = (struct pty_master *)this;
	struct pty *p = pm->pty;
	const uint8_t *d = buf;

	for (size_t i = 0; i < count; i++) {
		while (p->out.write_ptr == p->out.read_ptr + p->out.data_length) {
			event_trigger(&p->out.ev, false);
			struct event *events[] = {&p->out.ev};
			spinlock_drop(&this->lock);
			if (event_await(events, 1, true) < 0) {
				errno = EINTR;
				return -1;
			}
			spinlock_acquire_or_wait(&this->lock);
		}
		p->out.data[p->out.write_ptr++ % p->out.data_length] = d[i];
	}

	if (p->out.write_ptr == p->out.read_ptr + p->out.data_length) {
		this->status &= ~POLLOUT;
	}

	this->status |= POLLIN;

	event_trigger(&p->out.ev, false);
	spinlock_drop(&this->lock);
	return count;
}

static ssize_t pty_slave_read(struct resource *this,
							  struct f_description *description, void *buf,
							  off_t offset, size_t count) {

	(void)description;
	(void)offset;
	struct pty_slave *ps = (struct pty_slave *)this;
	struct pty *p = ps->pty;
	uint8_t *d = buf;

	if (p->out.read_ptr == p->out.write_ptr) {
		event_trigger(&p->out.ev, false);
		return 0;
	}

	spinlock_acquire_or_wait(&this->lock);

	size_t i = 0;
	for (i = 0; i < count; i++) {
		if (p->out.write_ptr == p->out.read_ptr) {
			this->status &= ~POLLIN;
			break;
		}
		d[i] = p->out.data[p->out.read_ptr++ % p->out.data_length];
	}

	if (p->out.read_ptr != p->out.write_ptr) {
		this->status |= POLLOUT;
	}

	event_trigger(&p->out.ev, false);
	spinlock_drop(&this->lock);
	return i;
}

static ssize_t pty_slave_write(struct resource *this,
							   struct f_description *description,
							   const void *buf, off_t offset, size_t count) {

	spinlock_acquire_or_wait(&this->lock);
	(void)description;
	(void)offset;

	struct pty_slave *ps = (struct pty_slave *)this;
	struct pty *p = ps->pty;
	const uint8_t *d = buf;

	for (size_t i = 0; i < count; i++) {
		while (p->in.write_ptr == p->in.read_ptr + p->in.data_length) {
			event_trigger(&p->in.ev, false);
			struct event *events[] = {&p->in.ev};
			spinlock_drop(&this->lock);
			if (event_await(events, 1, true) < 0) {
				errno = EINTR;
				return -1;
			}
			spinlock_acquire_or_wait(&this->lock);
		}
		p->in.data[p->in.write_ptr++ % p->in.data_length] = d[i];
	}

	if (p->in.write_ptr == p->in.read_ptr + p->in.data_length) {
		this->status &= ~POLLOUT;
	}

	this->status |= POLLIN;

	event_trigger(&p->in.ev, false);
	spinlock_drop(&this->lock);
	return count;
}

void syscall_openpty(struct syscall_arguments *args) {
	struct process *proc =
		prcb_return_current_cpu()->running_thread->mother_proc;

	int *fds = (int *)(syscall_helper_user_to_kernel_address(args->args0));
	if (fds == NULL) {
		errno = EFAULT;
		args->ret = -1;
		return;
	}

	struct pty *p = kmalloc(sizeof(struct pty));
	if (!p) {
		errno = ENOMEM;
		args->ret = -1;
		return;
	}

	p->name = pty_number++;

	p->in.data = kmalloc(PAGE_SIZE * 32);
	memzero(p->in.data, PAGE_SIZE * 32);
	p->in.data_length = PAGE_SIZE * 32;
	p->in.read_ptr = 0;
	p->in.write_ptr = 0;

	p->out.data = kmalloc(PAGE_SIZE * 32);
	memzero(p->out.data, PAGE_SIZE * 32);
	p->out.data_length = PAGE_SIZE * 32;
	p->out.read_ptr = 0;
	p->out.write_ptr = 0;

	p->term.c_iflag = BRKINT | IGNPAR | ICRNL | IXON | IMAXBEL;
	p->term.c_oflag = OPOST | ONLCR;
	p->term.c_cflag = CS8 | CREAD;
	p->term.c_lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE;
	p->term.c_cc[VINTR] = CTRL('C');
	p->term.c_cc[VEOF] = CTRL('D');
	p->term.c_cc[VSUSP] = CTRL('Z');

	p->term.ibaud = 38400;
	p->term.obaud = 38400;

	p->ws.ws_row = 80;
	p->ws.ws_col = 25;

	struct pty_slave *ps = resource_create(sizeof(struct pty_slave));
	struct pty_master *pm = resource_create(sizeof(struct pty_master));

	if (!ps || !pm) {
		errno = ENOMEM;
		args->ret = -1;
		return;
	}

	ps->pty = p;
	ps->res.read = pty_slave_read;
	ps->res.write = pty_slave_write;
	ps->res.ioctl = pty_ioctl;
	ps->res.status |= POLLOUT;

	pm->pty = p;
	pm->res.refcount = 1;
	pm->res.read = pty_master_read;
	pm->res.write = pty_master_write;
	pm->res.unref = pty_master_unref;
	pm->res.ioctl = pty_ioctl;
	pm->res.status |= POLLOUT;

	fds[0] = fdnum_create_from_resource(proc, (struct resource *)pm, O_RDWR, 0,
										false);
	fds[1] = fdnum_create_from_resource(proc, (struct resource *)ps, O_RDWR, 0,
										false);

	if (fds[0] == -1 || fds[1] == -1) {
		args->ret = -1;
	}

	args->ret = 0;
}