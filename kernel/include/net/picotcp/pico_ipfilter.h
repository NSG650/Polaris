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
#ifndef INCLUDE_PICO_IPFILTER
#define INCLUDE_PICO_IPFILTER

#include "pico_device.h"

enum filter_action {
	FILTER_PRIORITY = 0,
	FILTER_REJECT,
	FILTER_DROP,
	FILTER_COUNT
};

uint32_t pico_ipv4_filter_add(
	struct pico_device *dev, uint8_t proto, struct pico_ip4 *out_addr,
	struct pico_ip4 *out_addr_netmask, struct pico_ip4 *in_addr,
	struct pico_ip4 *in_addr_netmask, uint16_t out_port, uint16_t in_port,
	int8_t priority, uint8_t tos, enum filter_action action);

int pico_ipv4_filter_del(struct pico_stack *S, uint32_t filter_id);

int ipfilter(struct pico_frame *f);
int filter_compare(void *filterA, void *filterB);

#endif /* _INCLUDE_PICO_IPFILTER */
