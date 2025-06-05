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
#ifndef _INCLUDE_PICO_ND
#define _INCLUDE_PICO_ND
#include "pico_frame.h"
#include "pico_ipv6.h"

/* RFC constants */
#define PICO_ND_REACHABLE_TIME 30000 /* msec */
#define PICO_ND_RETRANS_TIMER 1000	 /* msec */

#define PICO_IPV6_ND_MIN_RADV_INTERVAL (5000)
#define PICO_IPV6_ND_MAX_RADV_INTERVAL (15000)

struct pico_nd_hostvars {
	uint8_t routing;
	uint8_t hoplimit;
	pico_time basetime;
	pico_time reachabletime;
	pico_time retranstime;
#ifdef PICO_SUPPORT_6LOWPAN
	uint8_t lowpan_flags;
#endif
};

int pico_ipv6_nd_qcompare(void *ka, void *kb);
void pico_ipv6_nd_init(struct pico_stack *S);
struct pico_eth *pico_ipv6_get_neighbor(struct pico_stack *S,
										struct pico_frame *f);
void pico_ipv6_nd_postpone(struct pico_stack *S, struct pico_frame *f);
int pico_ipv6_nd_recv(struct pico_frame *f);
void pico_ipv6_nd_ra_timer_callback(pico_time now, void *arg);

#ifdef PICO_SUPPORT_6LOWPAN
int pico_6lp_nd_start_soliciting(struct pico_ipv6_link *l,
								 struct pico_ipv6_route *gw);
void pico_6lp_nd_register(struct pico_ipv6_link *link);
#endif

#endif
