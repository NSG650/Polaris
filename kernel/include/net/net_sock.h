#ifndef NET_SOCK_H
#define NET_SOCK_H

#include <ipc/socket.h>
#include <klibc/vec.h>

struct net_socket {
    struct socket sock;
    int lwip_fd;
};

typedef vec_t(struct net_socket *) net_socket_vec_t;
struct socket *net_sock_create(int type, int protocol);

#endif