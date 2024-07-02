#ifndef UNIX_H
#define UNIX_H

#include <ipc/socket.h>

struct sockaddr_un {
	uint16_t sun_family;
	char sun_path[108];
};

struct unix_socket {
	struct socket sock;
	struct sockaddr_un name;
	struct unix_socket **backlog;
	size_t backlog_i;
	size_t backlog_max;
	uint8_t state;
	struct unix_socket *peer;
	uint8_t *data;
	size_t data_length;
	ssize_t capacity_used;
	uintptr_t read_ptr;
	uintptr_t write_ptr;
	struct event read_event;
	struct event write_event;
};

#define UNIX_SOCK_CONNECTED (1 << 0)
#define UNIX_SOCK_LISTENING (1 << 1)

struct socket *unix_sock_create(int family, int protocol);
struct socket **unix_sock_create_pair(int type, int protocol);

#endif
