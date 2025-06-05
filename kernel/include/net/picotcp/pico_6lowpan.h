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
#ifndef INCLUDE_PICO_6LOWPAN
#define INCLUDE_PICO_6LOWPAN

#include "pico_config.h"
#include "pico_device.h"
#include "pico_frame.h"
#include "pico_protocol.h"

#define PICO_6LP_FLAG_LOWPAN (0x01)
#define PICO_6LP_FLAG_NOMAC (0x02)

#ifdef PICO_SUPPORT_6LOWPAN
#define PICO_DEV_IS_6LOWPAN(dev) \
	((dev) && ((dev)->hostvars.lowpan_flags & PICO_6LP_FLAG_LOWPAN))
#define PICO_DEV_IS_NOMAC(dev) \
	((dev) && ((dev)->hostvars.lowpan_flags & PICO_6LP_FLAG_NOMAC))
#else
#define PICO_DEV_IS_6LOWPAN(dev) (0)
#define PICO_DEV_IS_NOMAC(dev) (0)
#endif

/******************************************************************************
 * Public variables
 ******************************************************************************/

extern struct pico_protocol pico_proto_6lowpan;

/******************************************************************************
 * Public functions
 ******************************************************************************/

/* Compares two fragmentation cookies according to RFC4944 5.3 */
int32_t lp_frag_ctx_cmp(void *a, void *b);
int32_t lp_frag_cmp(void *a, void *b);

int32_t pico_6lowpan_pull(struct pico_frame *f);
int pico_6lowpan_init(struct pico_stack *S);

#endif /* INCLUDE_PICO_6LOWPAN */
