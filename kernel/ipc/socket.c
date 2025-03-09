#include <debug/debug.h>
#include <errno.h>
#include <ipc/socket.h>
#include <ipc/unix.h>

struct socket *socket_create(int family, int type, int protocol) {
	switch (family) {
		case AF_UNIX:
			return unix_sock_create(type, protocol);
		default:
			return NULL;
	}
}

struct socket **socket_create_pair(int family, int type, int protocol) {
	switch (family) {
		case AF_UNIX:
			return unix_sock_create_pair(type, protocol);
		default:
			return NULL;
	}
}

void syscall_socket(struct syscall_arguments *args) {
	struct process *proc = sched_get_running_thread()->mother_proc;

	int family = (int)(args->args0);
	int type = (int)(args->args1);
	int protocol = (int)(args->args2);

	struct socket *sock = socket_create(family, type, protocol);

	if (sock == NULL) {
		errno = EINVAL;
		args->ret = -1;
		return;
	}

	int flags = 0;
	if (type & SOCK_CLOEXEC) {
		flags |= SOCK_CLOEXEC;
	}
	if (type & SOCK_NONBLOCK) {
		flags |= SOCK_NONBLOCK;
	}

	int ret = fdnum_create_from_resource(proc, (struct resource *)sock, flags,
										 0, false);

	if (ret == -1) {
		args->ret = -1;
		return;
	}

	args->ret = ret;
}

void syscall_socketpair(struct syscall_arguments *args) {
	struct process *proc = sched_get_running_thread()->mother_proc;

	int family = (int)(args->args0);
	int type = (int)(args->args1);
	int protocol = (int)(args->args2);
	int *fds = (int *)(args->args3);

	struct socket **sock = socket_create_pair(family, type, protocol);

	if (sock == NULL) {
		errno = EINVAL;
		args->ret = -1;
		return;
	}

	int flags = 0;
	if (type & SOCK_CLOEXEC) {
		flags |= SOCK_CLOEXEC;
	}
	if (type & SOCK_NONBLOCK) {
		flags |= SOCK_NONBLOCK;
	}

	fds[0] = fdnum_create_from_resource(proc, (struct resource *)sock[0], flags,
										0, false);
	fds[1] = fdnum_create_from_resource(proc, (struct resource *)sock[1], flags,
										0, false);

	if (fds[0] == -1 || fds[1] == -1) {
		args->ret = -1;
		return;
	}

	args->ret = 0;
}

void syscall_bind(struct syscall_arguments *args) {
	struct process *proc = sched_get_running_thread()->mother_proc;

	int fdnum = (int)(args->args0);
	void *addr = (void *)(args->args1);
	socklen_t len = (socklen_t)(args->args2);

	struct f_descriptor *fd = fd_from_fdnum(proc, fdnum);

	if (fd == NULL) {
		args->ret = -1;
		return;
	}

	struct f_description *desc = fd->description;
	desc->res->unref(desc->res, desc);

	if (!S_ISSOCK(desc->res->stat.st_mode)) {
		errno = ENOTSOCK;
		args->ret = -1;
		return;
	}

	struct socket *sock = (struct socket *)(desc->res);

	args->ret = sock->bind(sock, desc, addr, len) ? 0 : -1;
}

void syscall_connect(struct syscall_arguments *args) {
	struct process *proc = sched_get_running_thread()->mother_proc;

	int fdnum = (int)(args->args0);
	void *addr = (void *)(args->args1);
	socklen_t len = (socklen_t)(args->args2);

	struct f_descriptor *fd = fd_from_fdnum(proc, fdnum);

	if (fd == NULL) {
		args->ret = -1;
		return;
	}

	struct f_description *desc = fd->description;
	desc->res->unref(desc->res, desc);

	if (!S_ISSOCK(desc->res->stat.st_mode)) {
		errno = ENOTSOCK;
		args->ret = -1;
		return;
	}

	struct socket *sock = (struct socket *)(desc->res);

	args->ret = sock->connect(sock, desc, addr, len) ? 0 : -1;
}

void syscall_getpeername(struct syscall_arguments *args) {
	struct process *proc = sched_get_running_thread()->mother_proc;

	int fdnum = (int)(args->args0);
	void *addr = (void *)(args->args1);
	socklen_t *len = (socklen_t *)(args->args2);

	struct f_descriptor *fd = fd_from_fdnum(proc, fdnum);

	if (fd == NULL) {
		args->ret = -1;
		return;
	}

	struct f_description *desc = fd->description;
	desc->res->unref(desc->res, desc);

	if (!S_ISSOCK(desc->res->stat.st_mode)) {
		errno = ENOTSOCK;
		args->ret = -1;
		return;
	}

	struct socket *sock = (struct socket *)(desc->res);

	args->ret = sock->getpeername(sock, desc, addr, len) ? 0 : -1;
}

void syscall_listen(struct syscall_arguments *args) {
	struct process *proc = sched_get_running_thread()->mother_proc;

	int fdnum = (int)(args->args0);
	int backlog = (int)(args->args1);

	struct f_descriptor *fd = fd_from_fdnum(proc, fdnum);

	if (fd == NULL) {
		args->ret = -1;
		return;
	}

	struct f_description *desc = fd->description;
	desc->res->unref(desc->res, desc);

	if (!S_ISSOCK(desc->res->stat.st_mode)) {
		errno = ENOTSOCK;
		args->ret = -1;
		return;
	}

	struct socket *sock = (struct socket *)(desc->res);

	args->ret = sock->listen(sock, desc, backlog) ? 0 : -1;
}

void syscall_accept(struct syscall_arguments *args) {
	struct process *proc = sched_get_running_thread()->mother_proc;

	int fdnum = (int)(args->args0);

	struct f_descriptor *fd = fd_from_fdnum(proc, fdnum);

	if (fd == NULL) {
		args->ret = -1;
		return;
	}

	struct f_description *desc = fd->description;
	desc->res->unref(desc->res, desc);

	if (!S_ISSOCK(desc->res->stat.st_mode)) {
		errno = ENOTSOCK;
		args->ret = -1;
		return;
	}

	struct socket *sock = (struct socket *)(desc->res);
	struct socket *accep_sock = sock->accept(sock, desc);
	if (accep_sock == NULL) {
		args->ret = -1;
		return;
	}

	args->ret = fdnum_create_from_resource(proc, (struct resource *)accep_sock,
										   0, 0, false);
}

void syscall_recvmsg(struct syscall_arguments *args) {
	struct process *proc = sched_get_running_thread()->mother_proc;

	int fdnum = (int)(args->args0);
	struct msghdr *msg = (struct msghdr *)(args->args1);
	int flags = (int)(args->args2);

	struct f_descriptor *fd = fd_from_fdnum(proc, fdnum);

	if (fd == NULL) {
		args->ret = -1;
		return;
	}

	struct f_description *desc = fd->description;
	desc->res->unref(desc->res, desc);

	if (!S_ISSOCK(desc->res->stat.st_mode)) {
		errno = ENOTSOCK;
		args->ret = -1;
		return;
	}

	struct socket *sock = (struct socket *)(desc->res);
	args->ret = sock->recvmsg(sock, desc, msg, flags);
}
