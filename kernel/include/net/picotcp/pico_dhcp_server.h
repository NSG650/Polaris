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
#ifndef INCLUDE_PICO_DHCP_SERVER
#define INCLUDE_PICO_DHCP_SERVER
#include "pico_defines.h"
#ifdef PICO_SUPPORT_UDP

#include "pico_addressing.h"
#include "pico_dhcp_common.h"

struct pico_dhcp_server_setting {
	uint32_t pool_start;
	uint32_t pool_next;
	uint32_t pool_end;
	uint32_t lease_time;
	struct pico_device *dev;
	struct pico_socket *s;
	struct pico_ip4 server_ip;
	struct pico_ip4 netmask;
	uint8_t flags; /* unused atm */
};

int dhcp_settings_cmp(void *ka, void *kb);
int dhcp_negotiations_cmp(void *ka, void *kb);
/* required field: IP address of the interface to serve, only IPs of this
 * network will be served. */
int pico_dhcp_server_initiate(struct pico_dhcp_server_setting *dhcps);

/* To destroy an existing DHCP server configuration, running on a given
 * interface */
int pico_dhcp_server_destroy(struct pico_device *dev);

#endif /* _INCLUDE_PICO_DHCP_SERVER */
#endif
