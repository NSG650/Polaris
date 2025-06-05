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
#ifndef PICO_SOCKET_MULTICAST_H
#define PICO_SOCKET_MULTICAST_H

#include "pico_addressing.h"
#include "pico_socket.h"

int mcast_socket_cmp(void *ka, void *kb);
int mcast_filter_cmp(void *ka, void *kb);
int mcast_filter_cmp_ipv6(void *ka, void *kb);
int pico_socket_mcast_filter(struct pico_socket *s,
							 union pico_address *mcast_group,
							 union pico_address *src);
void pico_multicast_delete(struct pico_socket *s);
int pico_setsockopt_mcast(struct pico_socket *s, int option, void *value);
int pico_getsockopt_mcast(struct pico_socket *s, int option, void *value);
int pico_udp_get_mc_ttl(struct pico_socket *s, uint8_t *ttl);
int pico_udp_set_mc_ttl(struct pico_socket *s, void *_ttl);

#endif
