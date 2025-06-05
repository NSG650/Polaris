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
#ifndef INCLUDE_PICO_ADDRESSING
#define INCLUDE_PICO_ADDRESSING

#include "pico_config.h"
#include "pico_constants.h"

PACKED_STRUCT_DEF pico_ip4 {
	uint32_t addr;
};

PACKED_STRUCT_DEF pico_ip6 {
	uint8_t addr[16];
};

/******************************************************************************
 *  Ethernet Address Definitions
 ******************************************************************************/

PACKED_STRUCT_DEF pico_eth {
	uint8_t addr[6];
	uint8_t padding[2];
};

extern const uint8_t PICO_ETHADDR_ALL[];

/******************************************************************************
 *  Generic 6LoWPAN Address Definitions
 ******************************************************************************/

/* 6lowpan supports 16-bit short addresses */
PACKED_STRUCT_DEF pico_6lowpan_short {
	uint16_t addr;
};

/* And also EUI-64 addresses */
PACKED_STRUCT_DEF pico_6lowpan_ext {
	uint8_t addr[8];
};

PACKED_STRUCT_DEF pico_ll {
	uint16_t proto;
	uint16_t hatype;
	uint8_t pktype;
	uint8_t halen;
	struct pico_eth hwaddr;
	struct pico_device *dev;
};

union pico_address {
	struct pico_ip4 ip4;
	struct pico_ip6 ip6;
#ifdef PICO_SUPPORT_PACKET_SOCKETS
	struct pico_ll ll;
#endif
};

/* Address memory as either a short 16-bit address or a 64-bit address */
union pico_6lowpan_u {
	uint8_t data[8];
	struct pico_6lowpan_short _short;
	struct pico_6lowpan_ext _ext;
};

/* Info data structure to pass to pico_device_init by the device driver */
struct pico_6lowpan_info {
	struct pico_6lowpan_short addr_short;
	struct pico_6lowpan_ext addr_ext;
	struct pico_6lowpan_short pan_id;
};

/* Different addressing modes for IEEE802.15.4 addresses */
#define AM_6LOWPAN_NONE (0u)
#define AM_6LOWPAN_RES (1u)
#define AM_6LOWPAN_SHORT (2u)
#define AM_6LOWPAN_EXT (3u)
#define SIZE_6LOWPAN_SHORT (2u)
#define SIZE_6LOWPAN_EXT (8u)
#define SIZE_6LOWPAN(m) (((m) == 2) ? (2) : (((m) == 3) ? (8) : (0)))

/******************************************************************************
 *  Generic 6LoWPAN Address Definitions
 ******************************************************************************/

/* Storage data structure for IEEE802.15.4 addresses */
struct pico_802154 {
	union pico_6lowpan_u addr;
	uint8_t mode;
};

/******************************************************************************
 *  Link Layer addresses
 ******************************************************************************/

#define IID_16(iid) \
	(0 == (iid)[2] && 0xff == (iid)[3] && 0xfe == (iid)[4] && 0 == (iid)[5])

enum pico_ll_mode {
	LL_MODE_ETHERNET = 0,
#ifdef PICO_SUPPORT_802154
	LL_MODE_IEEE802154,
#endif
};

union pico_ll_addr {
	struct pico_eth eth;
	struct pico_802154 pan;
};

PACKED_STRUCT_DEF pico_trans {
	uint16_t sport;
	uint16_t dport;
};

/* Here are some protocols. */
#define PICO_PROTO_IPV4 0
#define PICO_PROTO_ICMP4 1
#define PICO_PROTO_IGMP 2
#define PICO_PROTO_TCP 6
#define PICO_PROTO_UDP 17
#define PICO_PROTO_IPV6 41
#define PICO_PROTO_ICMP6 58

/* And some special values used for raw sockets */
#define PICO_PROTO_RAWSOCKET (3 << 8)
#define PICO_RAWSOCKET_RAW 255

#endif
