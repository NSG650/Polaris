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
#ifndef INCLUDE_PICO_IGMP
#define INCLUDE_PICO_IGMP

#include "pico_ipv4.h"

#define PICO_IGMPV1 1
#define PICO_IGMPV2 2
#define PICO_IGMPV3 3

#define PICO_IGMP_STATE_CREATE 1
#define PICO_IGMP_STATE_UPDATE 2
#define PICO_IGMP_STATE_DELETE 3

#define PICO_IGMP_QUERY_INTERVAL 125

extern struct pico_protocol pico_proto_igmp;
int igmp_timer_cmp(void *ka, void *kb);
int igmp_parameters_cmp(void *ka, void *kb);
int igmp_sources_cmp(void *ka, void *kb);

int pico_igmp_state_change(struct pico_stack *S, struct pico_ip4 *mcast_link,
						   struct pico_ip4 *mcast_group, uint8_t filter_mode,
						   struct pico_tree *_MCASTFilter, uint8_t state);
#endif /* _INCLUDE_PICO_IGMP */
