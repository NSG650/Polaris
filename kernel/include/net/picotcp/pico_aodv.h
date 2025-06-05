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
#ifndef PICO_AODV_H_
#define PICO_AODV_H_

#include "pico_addressing.h"
#include "pico_config.h"
#include "pico_device.h"
#include <stdint.h>

/* RFC3561 */
#define PICO_AODV_PORT (654)

/* RFC3561 $10 */
#define AODV_ACTIVE_ROUTE_TIMEOUT \
	(8000u) /* Conservative value for link breakage detection */
#define AODV_DELETE_PERIOD \
	(5 * AODV_ACTIVE_ROUTE_TIMEOUT) /* Recommended value K = 5 */
#define AODV_ALLOWED_HELLO_LOSS (4) /* conservative */
#define AODV_NET_DIAMETER ((uint8_t)(35))
#define AODV_RREQ_RETRIES (2)
#define AODV_NODE_TRAVERSAL_TIME (40)
#define AODV_HELLO_INTERVAL (1000)
#define AODV_LOCAL_ADD_TTL 2
#define AODV_RREQ_RATELIMIT (10)
#define AODV_TIMEOUT_BUFFER (2)
#define AODV_TTL_START ((uint8_t)(1))
#define AODV_TTL_INCREMENT 2
#define AODV_TTL_THRESHOLD ((uint8_t)(7))
#define AODV_RERR_RATELIMIT (10)
#define AODV_MAX_REPAIR_TTL ((uint8_t)(AODV_NET_DIAMETER / 3))
#define AODV_MY_ROUTE_TIMEOUT (2 * AODV_ACTIVE_ROUTE_TIMEOUT)
#define AODV_NET_TRAVERSAL_TIME \
	(2 * AODV_NODE_TRAVERSAL_TIME * AODV_NET_DIAMETER)
#define AODV_BLACKLIST_TIMEOUT (AODV_RREQ_RETRIES * AODV_NET_TRAVERSAL_TIME)
#define AODV_NEXT_HOP_WAIT (AODV_NODE_TRAVERSAL_TIME + 10)
#define AODV_PATH_DISCOVERY_TIME (2 * AODV_NET_TRAVERSAL_TIME)
#define AODV_RING_TRAVERSAL_TIME(ttl) \
	(2 * AODV_NODE_TRAVERSAL_TIME * (ttl + AODV_TIMEOUT_BUFFER))
/* End section RFC3561 $10 */

#define AODV_TYPE_RREQ 1
#define AODV_TYPE_RREP 2
#define AODV_TYPE_RERR 3
#define AODV_TYPE_RACK 4

PACKED_STRUCT_DEF pico_aodv_rreq {
	uint8_t type;
	uint16_t req_flags;
	uint8_t hop_count;
	uint32_t rreq_id;
	uint32_t dest;
	uint32_t dseq;
	uint32_t orig;
	uint32_t oseq;
};

#define AODV_RREQ_FLAG_J 0x8000
#define AODV_RREQ_FLAG_R 0x4000
#define AODV_RREQ_FLAG_G 0x2000
#define AODV_RREQ_FLAG_D 0x1000
#define AODV_RREQ_FLAG_U 0x0800
#define AODV_RREQ_FLAG_RESERVED 0x07FF

PACKED_STRUCT_DEF pico_aodv_rrep {
	uint8_t type;
	uint8_t rep_flags;
	uint8_t prefix_sz;
	uint8_t hop_count;
	uint32_t dest;
	uint32_t dseq;
	uint32_t orig;
	uint32_t lifetime;
};

#define AODV_RREP_MAX_PREFIX 0x1F
#define AODV_RREP_FLAG_R 0x80
#define AODV_RREP_FLAG_A 0x40
#define AODV_RREP_FLAG_RESERVED 0x3F

#define PICO_AODV_NODE_NEW 0x0000
#define PICO_AODV_NODE_SYNC 0x0001
#define PICO_AODV_NODE_REQUESTING 0x0002
#define PICO_AODV_NODE_ROUTE_UP 0x0004
#define PICO_AODV_NODE_ROUTE_DOWN 0x0008
#define PICO_AODV_NODE_IDLING 0x0010
#define PICO_AODV_NODE_UNREACH 0x0020

#define PICO_AODV_ACTIVE(node)                  \
	((node->flags & PICO_AODV_NODE_ROUTE_UP) && \
	 (node->flags & PICO_AODV_NODE_ROUTE_DOWN))

struct pico_aodv_node {
	struct pico_stack *stack;
	union pico_address dest;
	pico_time last_seen;
	pico_time fwd_time;
	uint32_t dseq;
	uint16_t flags;
	uint8_t metric;
	uint8_t ring_ttl;
	uint8_t rreq_retry;
};

PACKED_STRUCT_DEF pico_aodv_unreachable {
	uint32_t addr;
	uint32_t dseq;
};

PACKED_STRUCT_DEF pico_aodv_rerr {
	uint8_t type;
	uint16_t rerr_flags;
	uint8_t dst_count;
	uint32_t unreach_addr;
	uint32_t unreach_dseq;
	struct pico_aodv_unreachable
		unreach[1]; /* unrechable nodes: must be at least 1. See dst_count field
					   above */
};

PACKED_STRUCT_DEF pico_aodv_rack {
	uint8_t type;
	uint8_t reserved;
};

int aodv_dev_cmp(void *ka, void *kb);
int aodv_node_compare(void *ka, void *kb);
int pico_aodv_init(struct pico_stack *S);
int pico_aodv_add(struct pico_stack *S, struct pico_device *dev);
int pico_aodv_lookup(struct pico_stack *S, const union pico_address *addr);
void pico_aodv_refresh(struct pico_stack *S, const union pico_address *addr);
#endif
