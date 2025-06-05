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
#ifndef INCLUDE_PICO_DHCP_CLIENT
#define INCLUDE_PICO_DHCP_CLIENT
#include "pico_defines.h"
#ifdef PICO_SUPPORT_UDP
#include "pico_addressing.h"
#include "pico_dhcp_common.h"
#include "pico_protocol.h"

struct pico_stack;
int dhcp_cookies_cmp(void *ka, void *kb);
int pico_dhcp_initiate_negotiation(struct pico_device *device,
								   void (*callback)(void *cli, int code),
								   uint32_t *xid);
void *pico_dhcp_get_identifier(struct pico_stack *S, uint32_t xid);
struct pico_ip4 pico_dhcp_get_address(void *cli);
struct pico_ip4 pico_dhcp_get_gateway(void *cli);
struct pico_ip4 pico_dhcp_get_netmask(void *cli);
struct pico_ip4 pico_dhcp_get_nameserver(void *cli, int index);
int pico_dhcp_client_abort(struct pico_stack *S, uint32_t xid);
char *pico_dhcp_get_hostname(struct pico_stack *S);
char *pico_dhcp_get_domain(struct pico_stack *S);

/* possible codes for the callback */
#define PICO_DHCP_SUCCESS 0
#define PICO_DHCP_ERROR 1
#define PICO_DHCP_RESET 2

/* Maximum size for Hostname/Domain name */
#define PICO_DHCP_HOSTNAME_MAXLEN 64U

#endif
#endif
