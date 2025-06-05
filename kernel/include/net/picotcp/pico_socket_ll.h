/*********************************************************************
 * PicoTCP-NG
 * Copyright (c) 2020 Daniele Lacamera <root@danielinux.net>
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
#ifndef INCLUDE_PICO_SOCKET_LL
#define INCLUDE_PICO_SOCKET_LL

#include "pico_config.h"
#include "pico_frame.h"
#include "pico_socket.h"
#include "pico_stack.h"

extern struct pico_protocol pico_proto_ll;

#define LL_HWADDR_SIZE 8
#define PICO_IDETH_ALL (0x0300)
#define PICO_AF_PACKET (0xAF17)
#define PICO_PACKET_TYPE_DGRAM (0x0001)
#define PICO_PACKET_TYPE_RAW (0x0003)

struct pico_ll_socket {
	struct pico_socket sock;
	uint16_t id;
	uint8_t type;
};

void pico_socket_set_raw(struct pico_socket *s);
int pico_socket_ll_compare(void *ka, void *kb);
int pico_socket_ll_process_in(struct pico_stack *S, struct pico_protocol *self,
							  struct pico_frame *f);
struct pico_frame *pico_ll_frame_alloc(struct pico_stack *S,
									   struct pico_protocol *self,
									   struct pico_device *dev, uint16_t size);
struct pico_socket *pico_socket_ll_open(struct pico_stack *S, uint16_t proto);
int pico_socket_ll_recvfrom(struct pico_socket *s, void *buf, uint32_t len,
							void *orig);
int pico_socket_ll_sendto(struct pico_socket *s, void *buf, uint32_t len,
						  void *dst);
int pico_setsockopt_ll(struct pico_socket *s, int option, void *value);
int pico_getsockopt_ll(struct pico_socket *s, int option, void *value);
int pico_socket_ll_close(struct pico_socket *arg);

#endif /* INCLUDE_PICO_SOCKET_LL */
