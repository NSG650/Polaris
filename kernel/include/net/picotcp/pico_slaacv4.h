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
#ifndef INCLUDE_PICO_SUPPORT_SLAACV4
#define INCLUDE_PICO_SUPPORT_SLAACV4
#include "pico_arp.h"

#define PICO_SLAACV4_SUCCESS 0
#define PICO_SLAACV4_ERROR 1

int pico_slaacv4_claimip(struct pico_device *dev,
						 void (*cb)(struct pico_ip4 *ip, uint8_t code));
void pico_slaacv4_unregisterip(void);

#endif /* _INCLUDE_PICO_SUPPORT_SLAACV4 */
