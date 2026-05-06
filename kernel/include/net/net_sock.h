#ifndef NET_SOCK_H
#define NET_SOCK_H

#include <ipc/socket.h>

struct net_socket {
    struct socket sock;
    int lwip_fd;
};

struct socket *net_sock_create(int type, int protocol);

#endif