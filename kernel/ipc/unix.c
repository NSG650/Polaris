#include <debug/debug.h>
#include <errno.h>
#include <ipc/unix.h>
#include <klibc/misc.h>
#include <sched/sched.h>

#define FIONREAD 0x541b

static bool unix_socket_add_to_backlog(struct unix_socket *sock,
									   struct unix_socket *other) {
	if (sock->backlog_i == sock->backlog_max) {
		errno = EAGAIN;
		return false;
	}

	sock->sock.res.status |= POLLIN;
	sock->backlog[sock->backlog_i++] = other;
	return true;
}

static ssize_t unix_sock_read(struct resource *_this,
							  struct f_description *description, void *buf,
							  off_t offset, size_t count) {
	(void)description;
	(void)offset;

	struct unix_socket *this = (struct unix_socket *)_this;
	ssize_t ret = -1;

	spinlock_acquire_or_wait(&this->sock.res.lock);

	while (this->capacity_used == 0) {
		if ((description->flags & O_NONBLOCK)) {
			errno = EAGAIN;
			goto end;
		}
		spinlock_drop(&this->sock.res.lock);
		struct event *events[] = {&this->sock.res.event};

		if (event_await(events, 1, true) < 0) {
			errno = EINTR;
			goto end;
		}
		spinlock_acquire_or_wait(&this->sock.res.lock);
	}

	if (this->capacity_used < (ssize_t)count) {
		count = this->capacity_used;
	}

	size_t before_wrap = 0, after_wrap = 0, new_ptr = 0;
	if (this->read_ptr + count > this->data_length) {
		before_wrap = this->data_length - this->read_ptr;
		after_wrap = count - before_wrap;
		new_ptr = after_wrap;
	} else {
		before_wrap = count;
		after_wrap = 0;
		new_ptr = this->read_ptr + count;

		if (new_ptr == this->data_length) {
			new_ptr = 0;
		}
	}

	memcpy(buf, this->data + this->read_ptr, before_wrap);
	if (after_wrap != 0) {
		memcpy(buf + before_wrap, this->data, after_wrap);
	}

	this->read_ptr = new_ptr;
	this->capacity_used -= count;
	this->peer->sock.res.status |= POLLOUT;

	event_trigger(&this->peer->sock.res.event, false);
	this->sock.res.status &= ~POLLIN;

	ret = count;
end:
	spinlock_drop(&this->sock.res.lock);
	return ret;
}

static ssize_t unix_sock_write(struct resource *_this,
							   struct f_description *description,
							   const void *buf, off_t offset, size_t count) {
	(void)description;
	(void)offset;

	struct unix_socket *this = (struct unix_socket *)_this;
	struct unix_socket *peer = this->peer;
	ssize_t ret = -1;
	spinlock_acquire_or_wait(&peer->sock.res.lock);

	while (peer->capacity_used == (ssize_t)peer->data_length) {
		if ((description->flags & O_NONBLOCK)) {
			errno = EAGAIN;
			goto end;
		}
		spinlock_drop(&peer->sock.res.lock);
		struct event *events[] = {&peer->sock.res.event};

		if (event_await(events, 1, true) < 0) {
			errno = EINTR;
			goto end;
		}
		spinlock_acquire_or_wait(&peer->sock.res.lock);
	}

	if (peer->capacity_used + count > peer->data_length) {
		count = peer->data_length - peer->capacity_used;
	}

	size_t before_wrap = 0, after_wrap = 0, new_ptr = 0;
	if (peer->write_ptr + count > peer->data_length) {
		before_wrap = peer->data_length - peer->write_ptr;
		after_wrap = count - before_wrap;
		new_ptr = after_wrap;
	} else {
		before_wrap = count;
		after_wrap = 0;
		new_ptr = peer->write_ptr + count;

		if (new_ptr == peer->data_length) {
			new_ptr = 0;
		}
	}

	memcpy(peer->data + peer->write_ptr, buf, before_wrap);
	if (after_wrap != 0) {
		memcpy(peer->data, buf + before_wrap, after_wrap);
	}

	peer->write_ptr = new_ptr;
	peer->capacity_used += count;
	peer->sock.res.status |= POLLIN;

	event_trigger(&peer->sock.res.event, false);
	ret = count;
end:
	spinlock_drop(&peer->sock.res.lock);
	return ret;
}

static int unix_sock_ioctl(struct resource *_this,
						   struct f_description *description, uint64_t request,
						   uint64_t arg) {
	struct unix_socket *this = (struct unix_socket *)_this;
	switch (request) {
		case FIONREAD: {
			if (this->state & UNIX_SOCK_LISTENING) {
				errno = EINVAL;
				return -1;
			}
			if (arg) {
				*(uint64_t *)(arg) = this->capacity_used;
			}
			return 0;
		}
		default:
			return resource_default_ioctl((struct resource *)this, description,
										  request, arg);
	}
}

static bool unix_sock_unref(struct resource *this,
							struct f_description *description) {
	(void)this;
	(void)description;
	return true;
}

static bool unix_sock_connect(struct socket *_this,
							  struct f_description *description, void *_addr,
							  socklen_t len) {
	(void)description;
	(void)len;

	struct unix_socket *this = (struct unix_socket *)_this;
	struct sockaddr_un *addr = _addr;

	if (addr->sun_family != AF_UNIX) {
		errno = EINVAL;
		return false;
	}

	struct process *proc = sched_get_running_thread()->mother_proc;
	struct vfs_node *node = vfs_get_node(proc->cwd, addr->sun_path, true);
	if (node == NULL) {
		return false;
	}

	if (!S_ISSOCK(node->resource->stat.st_mode)) {
		errno = ENOTSOCK;
		return false;
	}

	struct unix_socket *sock = (struct unix_socket *)node->resource;

	if (sock->sock.family != AF_UNIX) {
		errno = EINVAL;
		return false;
	}

	if (!(sock->state & UNIX_SOCK_LISTENING)) {
		errno = ECONNREFUSED;
		return false;
	}

	spinlock_acquire_or_wait(&sock->sock.res.lock);
	if (!unix_socket_add_to_backlog(sock, this)) {
		spinlock_drop(&sock->sock.res.lock);
		return false;
	}
	event_trigger(&sock->sock.res.event, false);
	spinlock_drop(&sock->sock.res.lock);

	struct event *conn_event[] = {&_this->connect_event};

	if (event_await(conn_event, 1, true) < 0) {
		errno = EINTR;
		return false;
	}

	event_trigger(&sock->sock.connect_event, false);
	this->sock.res.status |= POLLOUT;
	event_trigger(&_this->res.event, false);

	return true;
}

static bool unix_sock_getpeername(struct socket *_this,
								  struct f_description *description, void *addr,
								  socklen_t *len) {
	(void)description;

	struct unix_socket *this = (struct unix_socket *)_this;

	size_t actual_len = *len;
	if (actual_len < sizeof(struct sockaddr_un)) {
		actual_len = sizeof(struct sockaddr_un);
	}

	memcpy(addr, &this->name, actual_len);
	*len = actual_len;
	return true;
}

static bool unix_sock_listen(struct socket *_this,
							 struct f_description *description, int backlog) {
	(void)description;
	struct unix_socket *this = (struct unix_socket *)_this;
	this->state |= UNIX_SOCK_LISTENING;
	this->backlog_max = backlog;
	this->backlog = kmalloc(sizeof(struct unix_socket *) * backlog);
	return true;
}

static struct socket *unix_sock_accept(struct socket *_this,
									   struct f_description *description) {
	(void)description;
	struct unix_socket *this = (struct unix_socket *)_this;

	if (!(this->state & UNIX_SOCK_LISTENING)) {
		errno = EINVAL;
		return NULL;
	}

	if ((description->flags & O_NONBLOCK)) {
		errno = EAGAIN;
		return NULL;
	}

	spinlock_acquire_or_wait(&this->sock.res.lock);
	while (this->backlog_i == 0) {
		_this->res.status &= ~POLLIN;
		struct event *events[] = {&this->sock.res.event};

		spinlock_drop(&this->sock.res.lock);
		if (event_await(events, 1, true) < 0) {
			errno = EINTR;
			return NULL;
		}
		spinlock_acquire_or_wait(&this->sock.res.lock);
	}

	struct unix_socket *peer = this->backlog[0];
	for (size_t i = 1; i < this->backlog_i; i++) {
		this->backlog[i - 1] = this->backlog[i];
	}
	this->backlog_i--;
	struct unix_socket *connection_sock =
		(struct unix_socket *)unix_sock_create(peer->sock.type,
											   peer->sock.protocol);

	memcpy(&connection_sock->name, &peer->name, sizeof(struct sockaddr_un));
	connection_sock->state |= UNIX_SOCK_CONNECTED;
	connection_sock->peer = peer;

	peer->sock.res.refcount++;
	peer->peer = connection_sock;
	peer->state |= UNIX_SOCK_CONNECTED;

	if (this->backlog_i == 0) {
		_this->res.status &= ~POLLIN;
	}

	event_trigger(&peer->sock.connect_event, false);

	struct event *events[] = {&this->sock.connect_event};
	if (event_await(events, 1, true) == -1) {
		errno = EINTR;
		return NULL;
	}

	spinlock_drop(&this->sock.res.lock);
	return (struct socket *)connection_sock;
}

bool unix_sock_bind(struct socket *_this, struct f_description *description,
					void *addr_, socklen_t len) {
	(void)description;
	(void)len;

	struct unix_socket *this = (struct unix_socket *)_this;
	spinlock_acquire_or_wait(&this->sock.res.lock);
	struct process *proc = sched_get_running_thread()->mother_proc;

	struct sockaddr_un *addr = (struct sockaddr_un *)addr_;
	if (addr->sun_family != AF_UNIX) {
		errno = EINVAL;
		spinlock_drop(&this->sock.res.lock);
		return false;
	}

	struct vfs_node *node = vfs_create(proc->cwd, addr->sun_path, S_IFSOCK);
	if (node == NULL) {
		spinlock_drop(&this->sock.res.lock);
		return false;
	}

	_this->res.stat = node->resource->stat;
	node->resource = (struct resource *)this;
	this->name = *addr;
	spinlock_drop(&this->sock.res.lock);
	return true;
}

static ssize_t unix_sock_recvmsg(struct socket *_this,
								 struct f_description *description,
								 struct msghdr *msg, int flags) {
	if (flags) {
		errno = EINVAL;
		kprintf("Unix sockets don't support flags...");
		return -1;
	}
	struct unix_socket *this = (struct unix_socket *)_this;
	spinlock_acquire_or_wait(&this->sock.res.lock);

	size_t count = 0;
	for (size_t i = 0; i < msg->msg_iovlen; i++) {
		count += msg->msg_iov[i].iov_len;
	}

	while (this->capacity_used == 0) {
		this->peer->sock.res.status |= POLLOUT;

		event_trigger(&this->peer->sock.res.event, false);
		spinlock_drop(&this->sock.res.lock);

		if ((description->flags & O_NONBLOCK) != 0) {
			errno = EAGAIN;
			return -1;
		}

		struct event *events[] = {&this->sock.res.event};

		if (event_await(events, 1, true) == -1) {
			errno = EINTR;
			return -1;
		}

		spinlock_acquire_or_wait(&this->sock.res.lock);
	}

	if (this->capacity_used < (ssize_t)count) {
		count = this->capacity_used;
	}

	size_t before_wrap = 0, after_wrap = 0, new_ptr = 0;
	if (this->read_ptr + count > this->data_length) {
		before_wrap = this->data_length - this->read_ptr;
		after_wrap = count - before_wrap;
		new_ptr = after_wrap;
	} else {
		before_wrap = count;
		after_wrap = 0;
		new_ptr = this->read_ptr + count;

		if (new_ptr == this->data_length) {
			new_ptr = 0;
		}
	}

	void *tmp_buffer = kmalloc(before_wrap + after_wrap);
	if (tmp_buffer == NULL) {
		spinlock_drop(&this->sock.res.lock);
		errno = ENOMEM;
		return -1;
	}

	memcpy(tmp_buffer, this->data + this->read_ptr, before_wrap);
	if (after_wrap != 0) {
		memcpy(tmp_buffer + before_wrap, this->data, after_wrap);
	}

	size_t transferred = 0;
	size_t remaining = before_wrap + after_wrap;
	for (size_t i = 0; i < msg->msg_iovlen; i++) {
		size_t transfer_count = MIN(msg->msg_iov[i].iov_len, remaining);
		memcpy(msg->msg_iov[i].iov_base, tmp_buffer + transferred,
			   transfer_count);
		transferred += transfer_count;
		remaining -= transfer_count;
	}

	kfree(tmp_buffer);

	this->read_ptr = new_ptr;
	this->capacity_used -= transferred;
	this->peer->sock.res.status |= POLLOUT;

	event_trigger(&this->peer->sock.res.event, false);

	if (msg->msg_name != NULL && (this->state & UNIX_SOCK_CONNECTED)) {
		socklen_t actual_size = msg->msg_namelen;
		if (actual_size < sizeof(struct sockaddr_un)) {
			actual_size = sizeof(struct sockaddr_un);
		}

		memcpy(msg->msg_name, &this->peer->name, actual_size);
		msg->msg_namelen = actual_size;
	}

	this->sock.res.status &= ~POLLIN;

	spinlock_drop(&this->sock.res.lock);
	return transferred;
}

struct socket *unix_sock_create(int type, int protocol) {
	struct unix_socket *sock = resource_create(sizeof(struct unix_socket));

	if (sock == NULL) {
		return NULL;
	}

	sock->sock.res.stat.st_mode = S_IFSOCK;

	sock->sock.family = AF_UNIX;
	sock->sock.type = type;
	sock->sock.protocol = protocol;

	sock->sock.accept = unix_sock_accept;
	sock->sock.connect = unix_sock_connect;
	sock->sock.getpeername = unix_sock_getpeername;
	sock->sock.recvmsg = unix_sock_recvmsg;
	sock->sock.listen = unix_sock_listen;
	sock->sock.bind = unix_sock_bind;

	sock->sock.res.read = unix_sock_read;
	sock->sock.res.write = unix_sock_write;
	sock->sock.res.ioctl = unix_sock_ioctl;
	sock->sock.res.unref = unix_sock_unref;
	sock->sock.res.refcount = 1;

	sock->name.sun_family = AF_UNIX;
	sock->data = kmalloc(0x100000);
	sock->data_length = 0x100000;

	return (struct socket *)sock;
}

struct socket **unix_sock_create_pair(int type, int protocol) {
	struct unix_socket **sockets = kmalloc(sizeof(struct unix_socket) * 2);
	sockets[0] = (struct unix_socket *)unix_sock_create(type, protocol);
	sockets[1] = (struct unix_socket *)unix_sock_create(type, protocol);

	return (struct socket **)sockets;
}
