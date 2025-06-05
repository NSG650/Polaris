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
#ifndef INCLUDE_PICO_NAT
#define INCLUDE_PICO_NAT
#include "pico_frame.h"
#include "pico_ipv4.h"

#define PICO_NAT_PORT_FORWARD_DEL 0
#define PICO_NAT_PORT_FORWARD_ADD 1

int nat_cmp_inbound(void *ka, void *kb);
int nat_cmp_outbound(void *ka, void *kb);
#ifdef PICO_SUPPORT_NAT
void pico_ipv4_nat_print_table(struct pico_stack *S);
int pico_ipv4_nat_find(struct pico_stack *S, uint16_t nat_port,
					   struct pico_ip4 *src_addr, uint16_t src_port,
					   uint8_t proto);
int pico_ipv4_port_forward(struct pico_stack *S, struct pico_ip4 nat_addr,
						   uint16_t nat_port, struct pico_ip4 src_addr,
						   uint16_t src_port, uint8_t proto, uint8_t flag);

int pico_ipv4_nat_inbound(struct pico_stack *S, struct pico_frame *f,
						  struct pico_ip4 *link_addr);
int pico_ipv4_nat_outbound(struct pico_stack *S, struct pico_frame *f,
						   struct pico_ip4 *link_addr);
int pico_ipv4_nat_enable(struct pico_ipv4_link *link);
int pico_ipv4_nat_disable(void);
int pico_ipv4_nat_is_enabled(struct pico_ip4 *link_addr);
#else

#define pico_ipv4_nat_print_table() \
	do {                            \
	} while (0)
static inline int pico_ipv4_nat_inbound(struct pico_stack *S,
										struct pico_frame *f,
										struct pico_ip4 *link_addr) {
	(void)f;
	(void)link_addr;
	pico_err = PICO_ERR_EPROTONOSUPPORT;
	return -1;
}

static inline int pico_ipv4_nat_outbound(struct pico_stack *S,
										 struct pico_frame *f,
										 struct pico_ip4 *link_addr) {
	(void)f;
	(void)link_addr;
	pico_err = PICO_ERR_EPROTONOSUPPORT;
	return -1;
}

static inline int pico_ipv4_nat_enable(struct pico_ipv4_link *link) {
	(void)link;
	pico_err = PICO_ERR_EPROTONOSUPPORT;
	return -1;
}

static inline int pico_ipv4_nat_disable(void) {
	pico_err = PICO_ERR_EPROTONOSUPPORT;
	return -1;
}

static inline int pico_ipv4_nat_is_enabled(struct pico_ip4 *link_addr) {
	(void)link_addr;
	pico_err = PICO_ERR_EPROTONOSUPPORT;
	return -1;
}

static inline int pico_ipv4_nat_find(uint16_t nat_port,
									 struct pico_ip4 *src_addr,
									 uint16_t src_port, uint8_t proto) {
	(void)nat_port;
	(void)src_addr;
	(void)src_port;
	(void)proto;
	pico_err = PICO_ERR_EPROTONOSUPPORT;
	return -1;
}

static inline int pico_ipv4_port_forward(struct pico_ip4 nat_addr,
										 uint16_t nat_port,
										 struct pico_ip4 src_addr,
										 uint16_t src_port, uint8_t proto,
										 uint8_t flag) {
	(void)nat_addr;
	(void)nat_port;
	(void)src_addr;
	(void)src_port;
	(void)proto;
	(void)flag;
	pico_err = PICO_ERR_EPROTONOSUPPORT;
	return -1;
}
#endif

#endif /* _INCLUDE_PICO_NAT */
