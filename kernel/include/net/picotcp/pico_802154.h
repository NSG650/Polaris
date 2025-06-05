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
#ifndef INCLUDE_PICO_802154
#define INCLUDE_PICO_802154

#include "pico_6lowpan_ll.h"
#include "pico_config.h"
#include "pico_device.h"

/*******************************************************************************
 * Size definitions
 ******************************************************************************/

#define MTU_802154_PHY (128u)
#define MTU_802154_MAC (125u) // 127 - Frame Check Sequence

#define SIZE_802154_MHR_MIN (5u)
#define SIZE_802154_MHR_MAX (23u)
#define SIZE_802154_FCS (2u)
#define SIZE_802154_LEN (1u)
#define SIZE_802154_PAN (2u)

/*******************************************************************************
 * Structure definitions
 ******************************************************************************/

PACKED_STRUCT_DEF pico_802154_hdr {
	uint16_t fcf;
	uint8_t seq;
	uint16_t pan_id;
};

extern const struct pico_6lowpan_ll_protocol pico_6lowpan_ll_802154;

#endif /* INCLUDE_PICO_802154 */
