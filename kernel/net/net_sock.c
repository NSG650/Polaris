#include <net/net_sock.h>
#include "lwip/sockets.h"
#include <mm/slab.h>
#include <debug/debug.h>

extern net_socket_vec_t net_sockets_table;
extern lock_t net_sockets_table_lock;

struct sockaddr_linux {
    uint16_t sa_family;
    char     sa_data[];
};

static uint16_t net_sock_linux_to_lwip_sockaddr(void *addr, uint32_t addr_len) {
    struct sockaddr_linux *li = addr;
    struct sockaddr *lw = addr;
    uint16_t o = li->sa_family;
    lw->sa_len = addr_len;
    lw->sa_family = AF_INET;
    return o;
}

static void net_sock_lwip_to_linux_sockaddr(void *addr, uint16_t family) {
    struct sockaddr_linux *li = addr;
    li->sa_family = family;
}

static ssize_t net_sock_read(struct resource *_this,
                             struct f_description *description, void *buf,
                             off_t offset, size_t count) {
    (void)offset;
    struct net_socket *this = (struct net_socket *)_this;
    return lwip_read(this->lwip_fd, buf, count);
}

static ssize_t net_sock_write(struct resource *_this,
                              struct f_description *description,
                              const void *buf, off_t offset, size_t count) {
    (void)offset;
    struct net_socket *this = (struct net_socket *)_this;
    return lwip_write(this->lwip_fd, buf, count);
}

static int net_sock_ioctl(struct resource *_this,
                          struct f_description *description, uint64_t request,
                          uint64_t arg) {
    struct net_socket *this = (struct net_socket *)_this;
	return lwip_ioctl(this->lwip_fd, request, (void *)arg);
}

static bool net_sock_unref(struct resource *this,
                           struct f_description *description) {
    (void)description;

    struct net_socket *sock = (struct net_socket *)this;
    this->refcount--;
    if (this->refcount == 0) {
        spinlock_acquire_or_wait(&net_sockets_table_lock);
        vec_push(&net_sockets_table, sock);
        spinlock_drop(&net_sockets_table_lock);
        return (lwip_close(sock->lwip_fd) == 0);
    }
    return true;
}

static bool net_sock_connect(struct socket *_this,
                             struct f_description *description, void *addr,
                             socklen_t len) {
    (void)description;

    struct net_socket *this = (struct net_socket *)_this;
    uint16_t f = net_sock_linux_to_lwip_sockaddr(addr, len);
    if (lwip_connect(this->lwip_fd, addr, len) < 0) {
        net_sock_lwip_to_linux_sockaddr(addr, f);
        return false;
    }
    net_sock_lwip_to_linux_sockaddr(addr, f);
    this->sock.res.status |= POLLOUT;
    event_trigger(&this->sock.res.event, false);
    return true;
}

static bool net_sock_bind(struct socket *_this, struct f_description *description,
                          void *addr, socklen_t len) {
    (void)description;

    struct net_socket *this = (struct net_socket *)_this;
    uint16_t f = net_sock_linux_to_lwip_sockaddr(addr, len);
    int ret = lwip_bind(this->lwip_fd, addr, len);
    net_sock_lwip_to_linux_sockaddr(addr, f);
    return ret == 0;
}

static bool net_sock_listen(struct socket *_this,
                            struct f_description *description, int backlog) {
    (void)description;

    struct net_socket *this = (struct net_socket *)_this;
	return (lwip_listen(this->lwip_fd, backlog) == 0);
}

static ssize_t net_sock_recvmsg(struct socket *_this,
                                struct f_description *description,
                                struct msghdr *msg, int flags) {
    struct net_socket *this = (struct net_socket *)_this;

    if (description->flags & O_NONBLOCK) {
        flags |= MSG_DONTWAIT;
    }

    return lwip_recvmsg(this->lwip_fd, msg, flags);
}

static ssize_t net_sock_sendmsg(struct socket *_this, 
                                struct f_description *description, 
                                const struct msghdr *msg, int flags) {
    struct net_socket *this = (struct net_socket *)_this;

    if (description->flags & O_NONBLOCK)
        flags |= MSG_DONTWAIT;

    // for TCP, msg_name must be NULL - lwip_sendmsg rejects it otherwise
    if (msg->msg_name != NULL && this->sock.protocol == SOCK_STREAM) {
        errno = EISCONN;
        return -1;
    }

    // flatten iovec and use lwip_send for TCP
    if (this->sock.protocol == SOCK_STREAM) {
        ssize_t total = 0;
        for (size_t i = 0; i < msg->msg_iovlen; i++) {
            ssize_t ret = lwip_send(this->lwip_fd, 
                                    msg->msg_iov[i].iov_base,
                                    msg->msg_iov[i].iov_len, flags);
            if (ret < 0) return ret;
            total += ret;
        }
        return total;
    }

    return lwip_sendmsg(this->lwip_fd, msg, flags);
}

static ssize_t net_sock_getsockopt(struct socket *_this, struct f_description *description, 
									int level, int optname, void *optval, socklen_t *optlen) {
    (void)description;

    struct net_socket *this = (struct net_socket *)_this;
    return lwip_getsockopt(this->lwip_fd, level, optname, optval, optlen);
}

static ssize_t net_sock_setsockopt(struct socket *_this, struct f_description *description, 
									int level, int optname, const void *optval, socklen_t optlen) {
    (void)description;

    struct net_socket *this = (struct net_socket *)_this;
    return lwip_setsockopt(this->lwip_fd, level, optname, optval, optlen);
}

static bool net_sock_getsockname(struct socket *_this,
								  struct f_description *description, void *addr,
								  socklen_t *len) {
	(void)description;

	struct net_socket *this = (struct net_socket *)_this;
	return (lwip_getsockname(this->lwip_fd, addr, len) == 0);
}


static bool net_sock_getpeername(struct socket *_this,
								  struct f_description *description, void *addr,
								  socklen_t *len) {
	(void)description;

	struct net_socket *this = (struct net_socket *)_this;
	return (lwip_getpeername(this->lwip_fd, addr, len) == 0);
}

static struct socket *net_sock_accept(struct socket *_this,
									   struct f_description *description, void *addr, socklen_t *len) {
	struct net_socket *this = (struct net_socket *)_this;

	struct net_socket *sock = resource_create(sizeof(struct net_socket));
	if (sock == NULL) {
		return NULL;
	}

    uint16_t f = net_sock_linux_to_lwip_sockaddr(addr, *len);
	sock->lwip_fd = lwip_accept(this->lwip_fd, addr, len);
	if (sock->lwip_fd < 0) {
        net_sock_lwip_to_linux_sockaddr(addr, f);
		kfree(sock);
		return NULL;
	}
    net_sock_lwip_to_linux_sockaddr(addr, f);

	sock->sock.res.stat.st_mode = S_IFSOCK;

	sock->sock.family = AF_INET;
	sock->sock.protocol = this->sock.protocol;

	sock->sock.accept = net_sock_accept;
	sock->sock.connect = net_sock_connect;
    sock->sock.getsockname = net_sock_getsockname;
	sock->sock.getpeername = net_sock_getpeername;
    sock->sock.sendmsg = net_sock_sendmsg;
	sock->sock.recvmsg = net_sock_recvmsg;
	sock->sock.listen = net_sock_listen;
	sock->sock.bind = net_sock_bind;
    sock->sock.getsockopt = net_sock_getsockopt;
	sock->sock.setsockopt = net_sock_setsockopt;

	sock->sock.res.read = net_sock_read;
	sock->sock.res.write = net_sock_write;
	sock->sock.res.ioctl = net_sock_ioctl;
	sock->sock.res.unref = net_sock_unref;

    spinlock_acquire_or_wait(&net_sockets_table_lock);
    vec_push(&net_sockets_table, sock);
    spinlock_drop(&net_sockets_table_lock);

	return (struct socket *)sock;
}

struct socket *net_sock_create(int type, int protocol) {
    struct net_socket *sock = resource_create(sizeof(struct net_socket));

	if (sock == NULL) {
		return NULL;
	}

    sock->lwip_fd = lwip_socket(AF_INET, type, protocol);
	if (sock->lwip_fd < 0) {
		kfree(sock);
		return NULL;
	}

    sock->sock.res.stat.st_mode = S_IFSOCK;

	sock->sock.family = AF_INET;
	sock->sock.protocol = type;

	sock->sock.accept = net_sock_accept;
	sock->sock.connect = net_sock_connect;
    sock->sock.getsockname = net_sock_getsockname;
	sock->sock.getpeername = net_sock_getpeername;
    sock->sock.sendmsg = net_sock_sendmsg;
	sock->sock.recvmsg = net_sock_recvmsg;
	sock->sock.listen = net_sock_listen;
	sock->sock.bind = net_sock_bind;
    sock->sock.getsockopt = net_sock_getsockopt;
	sock->sock.setsockopt = net_sock_setsockopt;

	sock->sock.res.read = net_sock_read;
	sock->sock.res.write = net_sock_write;
	sock->sock.res.ioctl = net_sock_ioctl;
	sock->sock.res.unref = net_sock_unref;

    spinlock_acquire_or_wait(&net_sockets_table_lock);
    vec_push(&net_sockets_table, sock);
    spinlock_drop(&net_sockets_table_lock);

	return (struct socket *)sock;
}
