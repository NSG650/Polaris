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

#ifndef PICO_OLSR_H
#define PICO_OLSR_H

/* Objects */
struct olsr_route_entry {
	struct olsr_route_entry *next;
	uint32_t time_left;
	struct pico_ip4 destination;
	struct olsr_route_entry *gateway;
	struct pico_device *iface;
	uint16_t metric;
	uint8_t link_type;
	struct olsr_route_entry *children;
	uint16_t ansn;
	uint16_t seq;
	uint8_t lq, nlq;
	uint8_t advertised_tc;
};

void pico_olsr_init(struct pico_stack *S);
int pico_olsr_add(struct pico_device *dev);
struct olsr_route_entry *olsr_get_ethentry(struct pico_device *vif);
struct olsr_route_entry kill_neighbour(struct pico_stack *S, uint32_t loc_add,
									   uint32_t rem_add);
uint8_t olsr_set_nlq(struct pico_stack *S, struct pico_ip4 addr, uint8_t nlq);
#endif
