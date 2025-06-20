#include <debug/debug.h>
#include <devices/tty/pty.h>
#include <errno.h>
#include <fs/devtmpfs.h>
#include <klibc/mem.h>
#include <sched/sched.h>

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
			break;
		}
		case TIOCSWINSZ: {
			struct winsize *w = (void *)arg;
			if (w)
				p->ws = *w;
			break;
		}
		case TIOCGPGRP: {
			int *n = (int *)arg;
			if (n)
				*n = sched_get_running_thread()->mother_proc->pid;
			break;
		}
		case TIOCGSID: {
			int *n = (int *)arg;
			if (n)
				*n = sched_get_running_thread()->mother_proc->pid;
			break;
		}
		case TIOCGPTN: {
			int *n = (int *)arg;
			if (n)
				*n = p->name;
			break;
		}
		case TIOCSPTLCK: {
			break;
		}
		case TIOCSCTTY: {
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
	spinlock_acquire_or_wait(&this->lock);

	struct pty_master *pm = (struct pty_master *)this;
	struct pty *p = pm->pty;

	ssize_t ret = 0;

	while (p->in.used == 0) {
		if (description->flags & O_NONBLOCK) {
			goto end;
		}
		spinlock_drop(&this->lock);
		struct event *events[] = {&p->in.ev};
		if (event_await(events, 1, true) < 0) {
			errno = EINTR;
			ret = -1;
			return ret;
		}
		spinlock_acquire_or_wait(&this->lock);
	}

	if (p->in.used < count) {
		count = p->in.used;
	}

	size_t before_wrap = 0, after_wrap = 0, new_ptr = 0;
	if (p->in.read_ptr + count > p->in.data_length) {
		before_wrap = p->in.data_length - p->in.read_ptr;
		after_wrap = count - before_wrap;
		new_ptr = after_wrap;
	} else {
		before_wrap = count;
		after_wrap = 0;
		new_ptr = p->in.read_ptr + count;

		if (new_ptr == p->in.data_length) {
			new_ptr = 0;
		}
	}

	memcpy(buf, p->in.data + p->in.read_ptr, before_wrap);
	if (after_wrap) {
		memcpy(buf + before_wrap, p->in.data, after_wrap);
	}

	p->in.read_ptr = new_ptr;
	p->in.used -= count;
	if (p->in.used < p->in.data_length) {
		this->status |= POLLOUT;
		p->ps->res.status |= POLLOUT;
	}
	event_trigger(&p->in.ev, false);
	if (p->in.used == 0) {
		this->status &= ~POLLIN;
	}
	ret = count;
end:
	spinlock_drop(&this->lock);
	return ret;
}

static ssize_t pty_master_write(struct resource *this,
								struct f_description *description,
								const void *buf, off_t offset, size_t count) {
	(void)description;
	(void)offset;

	struct pty_master *pm = (struct pty_master *)this;
	struct pty *p = pm->pty;
	ssize_t ret = 0;

	spinlock_acquire_or_wait(&p->ps->res.lock);

	if (p->out.used == p->out.data_length) {
		spinlock_drop(&p->ps->res.lock);
		struct event *events[] = {&p->out.ev};
		if (event_await(events, 1, true) < 0) {
			errno = EINTR;
			ret = -1;
			return ret;
		}
		spinlock_acquire_or_wait(&p->ps->res.lock);
	}

	if (p->out.used + count > p->out.data_length) {
		count = p->out.data_length - p->out.used;
	}

	size_t before_wrap = 0, after_wrap = 0, new_ptr = 0;
	if (p->out.write_ptr + count > p->out.data_length) {
		before_wrap = p->out.data_length - p->out.write_ptr;
		after_wrap = count - before_wrap;
		new_ptr = after_wrap;
	} else {
		before_wrap = count;
		after_wrap = 0;
		new_ptr = p->out.write_ptr + count;

		if (new_ptr == p->out.data_length) {
			new_ptr = 0;
		}
	}

	memcpy(p->out.data + p->out.write_ptr, buf, before_wrap);
	if (after_wrap) {
		memcpy(p->out.data, buf + before_wrap, after_wrap);
	}

	p->out.write_ptr = new_ptr;
	p->out.used += count;

	if (p->out.used == p->out.data_length) {
		this->status &= ~POLLOUT;
	}

	this->status |= POLLIN;
	p->ps->res.status |= POLLIN;
	event_trigger(&p->out.ev, false);
	ret = count;

	spinlock_drop(&p->ps->res.lock);
	return ret;
}

static ssize_t pty_slave_read(struct resource *this,
							  struct f_description *description, void *buf,
							  off_t offset, size_t count) {
	(void)description;
	(void)offset;
	spinlock_acquire_or_wait(&this->lock);

	struct pty_slave *ps = (struct pty_slave *)this;
	struct pty *p = ps->pty;

	ssize_t ret = 0;

	while (p->out.used == 0) {
		if (description->flags & O_NONBLOCK) {
			goto end;
		}
		spinlock_drop(&this->lock);
		struct event *events[] = {&p->out.ev};
		if (event_await(events, 1, true) < 0) {
			errno = EINTR;
			ret = -1;
			return ret;
		}
		spinlock_acquire_or_wait(&this->lock);
	}

	if (p->out.used < count) {
		count = p->out.used;
	}

	size_t before_wrap = 0, after_wrap = 0, new_ptr = 0;
	if (p->out.read_ptr + count > p->out.data_length) {
		before_wrap = p->out.data_length - p->out.read_ptr;
		after_wrap = count - before_wrap;
		new_ptr = after_wrap;
	} else {
		before_wrap = count;
		after_wrap = 0;
		new_ptr = p->out.read_ptr + count;

		if (new_ptr == p->out.data_length) {
			new_ptr = 0;
		}
	}

	memcpy(buf, p->out.data + p->out.read_ptr, before_wrap);
	if (after_wrap) {
		memcpy(buf + before_wrap, p->out.data, after_wrap);
	}

	p->out.read_ptr = new_ptr;
	p->out.used -= count;

	if (p->out.used < p->out.data_length) {
		this->status |= POLLOUT;
		p->pm->res.status |= POLLOUT;
	}
	event_trigger(&p->out.ev, false);
	if (p->out.used == 0) {
		this->status &= ~POLLIN;
	}
	ret = count;
end:
	spinlock_drop(&this->lock);
	return ret;
}

static ssize_t pty_slave_write(struct resource *this,
							   struct f_description *description,
							   const void *buf, off_t offset, size_t count) {
	(void)description;
	(void)offset;

	struct pty_slave *ps = (struct pty_slave *)this;
	struct pty *p = ps->pty;
	ssize_t ret = 0;

	spinlock_acquire_or_wait(&p->pm->res.lock);
	if (p->in.used == p->in.data_length) {
		spinlock_drop(&p->pm->res.lock);
		struct event *events[] = {&p->in.ev};
		if (event_await(events, 1, true) < 0) {
			errno = EINTR;
			ret = -1;
			return ret;
		}
		spinlock_acquire_or_wait(&p->pm->res.lock);
	}

	if (p->in.used + count > p->in.data_length) {
		count = p->in.data_length - p->in.used;
	}

	size_t before_wrap = 0, after_wrap = 0, new_ptr = 0;
	if (p->in.write_ptr + count > p->in.data_length) {
		before_wrap = p->in.data_length - p->in.write_ptr;
		after_wrap = count - before_wrap;
		new_ptr = after_wrap;
	} else {
		before_wrap = count;
		after_wrap = 0;
		new_ptr = p->in.write_ptr + count;

		if (new_ptr == p->in.data_length) {
			new_ptr = 0;
		}
	}

	memcpy(p->in.data + p->in.write_ptr, buf, before_wrap);
	if (after_wrap) {
		memcpy(p->in.data, buf + before_wrap, after_wrap);
	}

	p->in.write_ptr = new_ptr;
	p->in.used += count;

	if (p->in.used == p->in.data_length) {
		this->status &= ~POLLOUT;
	}
	this->status |= POLLIN;
	p->pm->res.status |= POLLIN;
	event_trigger(&p->in.ev, false);
	ret = count;

	spinlock_drop(&p->pm->res.lock);
	return ret;
}

void syscall_openpty(struct syscall_arguments *args) {
	struct process *proc = sched_get_running_thread()->mother_proc;

	int *fds = (int *)(args->args0);
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
	p->in.used = 0;
	p->in.read_ptr = 0;
	p->in.write_ptr = 0;

	p->out.data = kmalloc(PAGE_SIZE * 32);
	memzero(p->out.data, PAGE_SIZE * 32);
	p->out.data_length = PAGE_SIZE * 32;
	p->out.used = 0;
	p->out.read_ptr = 0;
	p->out.write_ptr = 0;

	p->term.c_iflag = BRKINT | IGNPAR | ICRNL | IXON | IMAXBEL;
	p->term.c_oflag = OPOST | ONLCR;
	p->term.c_cflag = CS8 | CREAD | 0x04;
	p->term.c_lflag = ISIG | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE;
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
	ps->res.stat.st_size = 0;
	ps->res.stat.st_blocks = 0;
	ps->res.stat.st_blksize = 4096;
	ps->res.stat.st_rdev = resource_create_dev_id();
	ps->res.stat.st_mode = 0644 | S_IFCHR;

	pm->pty = p;
	pm->res.refcount = 1;
	pm->res.read = pty_master_read;
	pm->res.write = pty_master_write;
	pm->res.unref = pty_master_unref;
	pm->res.ioctl = pty_ioctl;

	p->ps = ps;
	p->pm = pm;

	fds[0] = fdnum_create_from_resource(proc, (struct resource *)pm, O_RDWR, 0,
										false);
	fds[1] = fdnum_create_from_resource(proc, (struct resource *)ps, O_RDWR, 0,
										false);

	if (fds[0] == -1 || fds[1] == -1) {
		args->ret = -1;
	}

	char name[5 + 3 + 1] = "pty";
	char num[3 + 1] = {0};
	ultoa(p->name, num, 10);
	strcat(name, num);

	devtmpfs_add_device((struct resource *)ps, name);

	args->ret = 0;
}