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
#ifndef INCLUDE_PICO_ARP
#define INCLUDE_PICO_ARP
#include "pico_device.h"
#include "pico_eth.h"
#include "pico_stack.h"

int pico_arp_receive(struct pico_frame *);

struct pico_eth *pico_arp_get(struct pico_stack *S, struct pico_frame *f);
int32_t pico_arp_request(struct pico_device *dev, struct pico_ip4 *dst,
						 uint8_t type);

#define PICO_ARP_STATUS_REACHABLE 0x00
#define PICO_ARP_STATUS_PERMANENT 0x01
#define PICO_ARP_STATUS_STALE 0x02

#define PICO_ARP_QUERY 0x00
#define PICO_ARP_PROBE 0x01
#define PICO_ARP_ANNOUNCE 0x02

#define PICO_ARP_CONFLICT_REASON_CONFLICT 0
#define PICO_ARP_CONFLICT_REASON_PROBE 1

struct pico_eth *pico_arp_lookup(struct pico_stack *S, struct pico_ip4 *dst);
struct pico_ip4 *pico_arp_reverse_lookup(struct pico_stack *S,
										 struct pico_eth *dst);
int pico_arp_create_entry(uint8_t *hwaddr, struct pico_ip4 ipv4,
						  struct pico_device *dev);
void pico_arp_register_ipconflict(struct pico_stack *S, struct pico_ip4 *ip,
								  struct pico_eth *mac,
								  void (*cb)(struct pico_stack *S, int reason));
void pico_arp_postpone(struct pico_frame *f);
void pico_arp_init(struct pico_stack *S);
int arp_compare(void *ka, void *kb);
#endif
