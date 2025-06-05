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
#ifndef PICO_SOCKET_TCP_H
#define PICO_SOCKET_TCP_H
#include "pico_socket.h"

#ifdef PICO_SUPPORT_TCP

/* Functions/macros: conditional! */

#define IS_NAGLE_ENABLED(s) \
	(!(!(!(s->opt_flags & (1 << PICO_SOCKET_OPT_TCPNODELAY)))))
int pico_setsockopt_tcp(struct pico_socket *s, int option, void *value);
int pico_getsockopt_tcp(struct pico_socket *s, int option, void *value);
int pico_socket_tcp_deliver(struct pico_sockport *sp, struct pico_frame *f);
void pico_socket_tcp_delete(struct pico_socket *s);
void pico_socket_tcp_cleanup(struct pico_socket *sock);
struct pico_socket *pico_socket_tcp_open(struct pico_stack *S, uint16_t family);
int pico_socket_tcp_read(struct pico_socket *s, void *buf, uint32_t len);
void transport_flags_update(struct pico_frame *, struct pico_socket *);

#else
#define pico_getsockopt_tcp(...) (-1)
#define pico_setsockopt_tcp(...) (-1)
#define pico_socket_tcp_deliver(...) (-1)
#define IS_NAGLE_ENABLED(s) (0)
#define pico_socket_tcp_delete(...) \
	do {                            \
	} while (0)
#define pico_socket_tcp_cleanup(...) \
	do {                             \
	} while (0)
#define pico_socket_tcp_open(f) (NULL)
#define pico_socket_tcp_read(...) (-1)
#define transport_flags_update(...) \
	do {                            \
	} while (0)

#endif

#endif
