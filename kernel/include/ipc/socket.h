#ifndef SOCKET_H
#define SOCKET_H

#include <klibc/resource.h>

struct iovec {
	void *iov_base;
	size_t iov_len;
};

struct msghdr {
	void *msg_name;
	socklen_t msg_namelen;

	struct iovec *msg_iov;
	size_t msg_iovlen;

	void *msg_control;
	size_t msg_controllen;
	int msg_flags;
};

struct socket {
	struct resource res;

	int family;
	int type;
	int protocol;
	struct event connect_event;

	bool (*bind)(struct socket *this, struct f_description *description,
				 void *addr, socklen_t len);
	bool (*connect)(struct socket *this, struct f_description *description,
					void *addr, socklen_t len);
	bool (*getpeername)(struct socket *this, struct f_description *description,
						void *addr, socklen_t *len);
	bool (*listen)(struct socket *this, struct f_description *description,
				   int backlog);
	struct socket *(*accept)(struct socket *this,
							 struct f_description *description);
	ssize_t (*recvmsg)(struct socket *this, struct f_description *description,
					   struct msghdr *msg, int flags);
};

#define AF_UNIX 1
#define AF_LOCAL 1

void syscall_socket(struct syscall_arguments *args);
void syscall_socketpair(struct syscall_arguments *args);
void syscall_bind(struct syscall_arguments *args);
void syscall_connect(struct syscall_arguments *args);
void syscall_getpeername(struct syscall_arguments *args);
void syscall_listen(struct syscall_arguments *args);
void syscall_accept(struct syscall_arguments *args);
void syscall_recvmsg(struct syscall_arguments *args);

#endif
