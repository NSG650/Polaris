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
#ifndef INCLUDE_PICO_MCAST
#define INCLUDE_PICO_MCAST

#define MCAST_MODE_IS_INCLUDE (1)
#define MCAST_MODE_IS_EXCLUDE (2)
#define MCAST_CHANGE_TO_INCLUDE_MODE (3)
#define MCAST_CHANGE_TO_EXCLUDE_MODE (4)
#define MCAST_ALLOW_NEW_SOURCES (5)
#define MCAST_BLOCK_OLD_SOURCES (6)
#define MCAST_EVENT_DELETE_GROUP (0x0)
#define MCAST_EVENT_CREATE_GROUP (0x1)
#define MCAST_EVENT_UPDATE_GROUP (0x2)
#define MCAST_EVENT_QUERY_RECV (0x3)
#define MCAST_EVENT_REPORT_RECV (0x4)
#define MCAST_EVENT_TIMER_EXPIRED (0x5)
#define MCAST_NO_REPORT (1)

struct mcast_parameters {
	uint8_t event;
	uint8_t state;
	uint8_t general_query;
	uint8_t filter_mode;
	uint8_t last_host;
	uint16_t max_resp_time;
	union pico_address mcast_link;
	union pico_address mcast_group;
	struct pico_tree *MCASTFilter;
	struct pico_frame *f;
	struct pico_stack *stack;
};

struct pico_mcast_group {
	uint8_t filter_mode;
	uint16_t reference_count;
	union pico_address mcast_addr;
	struct pico_tree MCASTSources;
	struct pico_stack *stack;
};

struct mcast_filter_parameters {
	struct mcast_parameters *p;
	struct pico_tree *allow;
	struct pico_tree *block;
	struct pico_tree *filter;
	uint16_t sources;
	uint8_t proto;
	uint8_t record_type;
	struct pico_mcast_group *g;
	union pico_link *link;
};

extern int8_t pico_mcast_generate_filter(struct pico_stack *S,
										 struct mcast_filter_parameters *filter,
										 struct mcast_parameters *p);

#endif
