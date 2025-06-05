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
#ifndef INCLUDE_PICO_DNS_CLIENT
#define INCLUDE_PICO_DNS_CLIENT

#define PICO_DNS_NS_DEL 0
#define PICO_DNS_NS_ADD 1
#include "pico_config.h"
#include "pico_stack.h"
/* Compression values */
#define PICO_DNS_LABEL 0
#define PICO_DNS_POINTER 3

/* Label len */
#define PICO_DNS_LABEL_INITIAL 1u
#define PICO_DNS_LABEL_ROOT 1

/* TTL values */
#define PICO_DNS_MAX_TTL 604800 /* one week */

/* Len of an IPv4 address string */
#define PICO_DNS_IPV4_ADDR_LEN 16
#define PICO_DNS_IPV6_ADDR_LEN 54

/* Default nameservers + port */
#define PICO_DNS_NS_DEFAULT "208.67.222.222"
#define PICO_DNS_NS_PORT 53

/* RDLENGTH for A and AAAA RR's */
#define PICO_DNS_RR_A_RDLENGTH 4
#define PICO_DNS_RR_AAAA_RDLENGTH 16
int dns_query_cmp(void *ka, void *kb);
int dns_nameserver_cmp(void *ka, void *kb);

int pico_dns_client_init(struct pico_stack *S);
/* flag is PICO_DNS_NS_DEL or PICO_DNS_NS_ADD */
int pico_dns_client_nameserver(struct pico_stack *S, struct pico_ip4 *ns,
							   uint8_t flag);
int pico_dns_client_getaddr(struct pico_stack *S, const char *url,
							void (*callback)(char *ip, void *arg), void *arg);
int pico_dns_client_getname(struct pico_stack *S, const char *ip,
							void (*callback)(char *url, void *arg), void *arg);
#ifdef PICO_SUPPORT_IPV6
int pico_dns_client_getaddr6(struct pico_stack *S, const char *url,
							 void (*callback)(char *, void *), void *arg);
int pico_dns_client_getname6(struct pico_stack *S, const char *url,
							 void (*callback)(char *, void *), void *arg);
#endif

#endif /* _INCLUDE_PICO_DNS_CLIENT */
