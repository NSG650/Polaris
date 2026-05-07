#ifndef SOCKET_H
#define SOCKET_H

#include <klibc/resource.h>

struct msghdr;

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
	bool (*getsockname)(struct socket *this, struct f_description *description, 
						void *addr, socklen_t *len);
	bool (*getpeername)(struct socket *this, struct f_description *description,
						void *addr, socklen_t *len);
	bool (*listen)(struct socket *this, struct f_description *description,
				   int backlog);
	struct socket *(*accept)(struct socket *this,
							 struct f_description *description, void *addr, socklen_t *len);
	ssize_t (*recvmsg)(struct socket *this, struct f_description *description,
					   struct msghdr *msg, int flags);
	ssize_t (*sendmsg)(struct socket *this, struct f_description *description, 
						const struct msghdr *msg, int flags);
	ssize_t (*getsockopt)(struct socket *this, struct f_description *description, 
						  int level, int optname, void *optval, socklen_t *optlen);
    ssize_t (*setsockopt)(struct socket *this, struct f_description *description, int level, 
		                  int optname, const void *optval, socklen_t optlen);
};

#define AF_UNIX 1
#define AF_LOCAL 1
#define AF_INET 2

#define SOCK_STREAM	1
#define SOCK_DGRAM 2

void syscall_socket(struct syscall_arguments *args);
void syscall_socketpair(struct syscall_arguments *args);
void syscall_bind(struct syscall_arguments *args);
void syscall_connect(struct syscall_arguments *args);
void syscall_getsockname(struct syscall_arguments *args);
void syscall_getpeername(struct syscall_arguments *args);
void syscall_listen(struct syscall_arguments *args);
void syscall_accept(struct syscall_arguments *args);
void syscall_recvmsg(struct syscall_arguments *args);
void syscall_sendmsg(struct syscall_arguments *args);
void syscall_getsockopt(struct syscall_arguments *args);
void syscall_setsockopt(struct syscall_arguments *args);

#endif
