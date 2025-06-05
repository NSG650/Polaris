/*********************************************************************
 * PicoTCP-NG
 * Copyright (c) 2020 Daniele Lacamera <root@danielinux.net>
 *
 * This file also includes code from:
 * PicoTCP
 * Copyright (c) 2012-2017 Altran Intelligent Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
 *
 * PicoTCP-NG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) version 3.
 *
 * PicoTCP-NG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 *
 *
 *********************************************************************/
#ifndef INCLUDE_PICO_PROTOCOL
#define INCLUDE_PICO_PROTOCOL
#include "pico_config.h"
#include "pico_queue.h"
#include "pico_tree.h"

#define PICO_LOOP_DIR_IN 1
#define PICO_LOOP_DIR_OUT 2
struct pico_stack;

enum pico_layer {
	PICO_LAYER_DATALINK = 2, /* Ethernet only. */
	PICO_LAYER_NETWORK =
		3, /* IPv4, IPv6, ARP. Arp is there because it communicates with L2 */
	PICO_LAYER_TRANSPORT = 4, /* UDP, TCP, ICMP */
	PICO_LAYER_SOCKET = 5	  /* Socket management */
};

enum pico_err_e {
	PICO_ERR_NOERR = 0,
	PICO_ERR_EPERM = 1,
	PICO_ERR_ENOENT = 2,
	/* ... */
	PICO_ERR_EINTR = 4,
	PICO_ERR_EIO = 5,
	PICO_ERR_ENXIO = 6,
	/* ... */
	PICO_ERR_EAGAIN = 11,
	PICO_ERR_ENOMEM = 12,
	PICO_ERR_EACCESS = 13,
	PICO_ERR_EFAULT = 14,
	/* ... */
	PICO_ERR_EBUSY = 16,
	PICO_ERR_EEXIST = 17,
	/* ... */
	PICO_ERR_EINVAL = 22,
	/* ... */
	PICO_ERR_ENONET = 64,
	/* ... */
	PICO_ERR_EPROTO = 71,
	/* ... */
	PICO_ERR_ENOPROTOOPT = 92,
	PICO_ERR_EPROTONOSUPPORT = 93,
	/* ... */
	PICO_ERR_EOPNOTSUPP = 95,
	PICO_ERR_EADDRINUSE = 98,
	PICO_ERR_EADDRNOTAVAIL = 99,
	PICO_ERR_ENETDOWN = 100,
	PICO_ERR_ENETUNREACH = 101,
	/* ... */
	PICO_ERR_ECONNRESET = 104,
	/* ... */
	PICO_ERR_EISCONN = 106,
	PICO_ERR_ENOTCONN = 107,
	PICO_ERR_ESHUTDOWN = 108,
	/* ... */
	PICO_ERR_ETIMEDOUT = 110,
	PICO_ERR_ECONNREFUSED = 111,
	PICO_ERR_EHOSTDOWN = 112,
	PICO_ERR_EHOSTUNREACH = 113,
	PICO_ERR_EMSGSIZE = 114,
	PICO_ERR_EINPROGRESS = 115,
};

typedef enum pico_err_e pico_err_t;
extern PICO_THREAD_LOCAL volatile pico_err_t pico_err;

#define IS_IPV6(f) \
	(f && f->net_hdr && ((((uint8_t *)(f->net_hdr))[0] & 0xf0) == 0x60))
#define IS_IPV4(f) \
	(f && f->net_hdr && ((((uint8_t *)(f->net_hdr))[0] & 0xf0) == 0x40))

#define MAX_PROTOCOL_NAME 16

struct pico_proto_rr {
	struct pico_tree *t;
	struct pico_tree_node *node_in, *node_out;
};

struct pico_scheduler {
	struct pico_tree Datalink_proto_tree, Network_proto_tree,
		Transport_proto_tree, Socket_proto_tree;
	struct pico_proto_rr rr_datalink, rr_network, rr_transport, rr_socket;
};

struct pico_protocol {
	char name[MAX_PROTOCOL_NAME];
	uint32_t hash;
	enum pico_layer layer;
	uint16_t proto_number;
	struct pico_queue *q_in;
	struct pico_queue *q_out;
	struct pico_frame *(*alloc)(struct pico_stack *S,
								struct pico_protocol *self,
								struct pico_device *dev,
								uint16_t size); /* Frame allocation. */
	int (*push)(struct pico_stack *S, struct pico_protocol *self,
				struct pico_frame *
					p); /* Push function, for active outgoing pkts from above */
	int (*process_out)(struct pico_stack *S, struct pico_protocol *self,
					   struct pico_frame *p); /* Send loop. */
	int (*process_in)(struct pico_stack *S, struct pico_protocol *self,
					  struct pico_frame *p); /* Recv loop. */
	uint16_t (*get_mtu)(struct pico_stack *S, struct pico_protocol *self);
};

int pico_protocols_loop(int loop_score);
int pico_protocol_scheduler_init(struct pico_stack *S);
void pico_protocol_init(struct pico_stack *S, struct pico_protocol *p);

int pico_protocol_datalink_loop(struct pico_stack *S, int loop_score,
								int direction);
int pico_protocol_network_loop(struct pico_stack *S, int loop_score,
							   int direction);
int pico_protocol_transport_loop(struct pico_stack *S, int loop_score,
								 int direction);
int pico_protocol_socket_loop(struct pico_stack *S, int loop_score,
							  int direction);

#endif
