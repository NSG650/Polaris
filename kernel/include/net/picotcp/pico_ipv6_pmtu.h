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
#ifndef _INCLUDE_PICO_IPV6_PMTU
#define _INCLUDE_PICO_IPV6_PMTU

#include "pico_addressing.h"
#include "pico_stack.h"
#define PICO_PMTU_OK (0)
#define PICO_PMTU_ERROR (-1)
#define PICO_PMTU_CACHE_CLEANUP_INTERVAL (10 * (60 * 1000))

struct pico_ipv6_path_id {
	struct pico_ip6 dst;
};

struct pico_ipv6_path_mtu {
	struct pico_ipv6_path_id path;
	uint32_t mtu;
	int cache_status;
};

struct pico_ipv6_path_timer {
	pico_time interval;
	uint32_t id;
};

int pico_ipv6_path_compare(void *ka, void *kb);
uint32_t pico_ipv6_pmtu_get(struct pico_stack *S,
							const struct pico_ipv6_path_id *path);
int pico_ipv6_path_add(struct pico_stack *S,
					   const struct pico_ipv6_path_id *path, uint32_t mtu);
int pico_ipv6_path_update(struct pico_stack *S,
						  const struct pico_ipv6_path_id *path, uint32_t mtu);
int pico_ipv6_path_del(struct pico_stack *S,
					   const struct pico_ipv6_path_id *path);
void pico_ipv6_path_init(struct pico_stack *S, pico_time interval);

#endif
