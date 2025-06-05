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
#ifndef PICO_SOCKET_UDP_H
#define PICO_SOCKET_UDP_H

#include "pico_frame.h"
#include "pico_socket.h"

struct pico_socket *pico_socket_udp_open(struct pico_stack *S);
int pico_socket_udp_deliver(struct pico_sockport *sp, struct pico_frame *f);

#ifdef PICO_SUPPORT_UDP
int pico_setsockopt_udp(struct pico_socket *s, int option, void *value);
int pico_getsockopt_udp(struct pico_socket *s, int option, void *value);
#define pico_socket_udp_recv(s, buf, len, addr, port) \
	pico_udp_recv(s, buf, len, addr, port, NULL)
#else
#define pico_socket_udp_recv(...) (0)
#define pico_getsockopt_udp(...) (-1)
#define pico_setsockopt_udp(...) (-1)
#endif

#endif
