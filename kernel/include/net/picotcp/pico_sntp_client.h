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
#ifndef INCLUDE_PICO_SNTP_CLIENT
#define INCLUDE_PICO_SNTP_CLIENT

#include "pico_config.h"
#include "pico_protocol.h"

struct pico_timeval {
	pico_time tv_sec;
	pico_time tv_msec;
};

int pico_sntp_sync(struct pico_stack *S, const char *sntp_server,
				   void (*cb_synced)(pico_err_t status));
int pico_sntp_sync_ip(struct pico_stack *S, union pico_address *sntp_addr,
					  void (*cb_synced)(pico_err_t status));
int pico_sntp_gettimeofday(struct pico_stack *S, struct pico_timeval *tv);

#endif /* _INCLUDE_PICO_SNTP_CLIENT */
