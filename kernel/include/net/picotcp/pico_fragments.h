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
#ifndef PICO_FRAGMENTS_H
#define PICO_FRAGMENTS_H
#include "pico_addressing.h"
#include "pico_frame.h"
#include "pico_ipv4.h"
#include "pico_ipv6.h"

int pico_ipv6_frag_compare(void *ka, void *kb);
int pico_ipv4_frag_compare(void *ka, void *kb);
void pico_ipv6_process_frag(struct pico_ipv6_exthdr *frag, struct pico_frame *f,
							uint8_t proto);
void pico_ipv4_process_frag(struct pico_ipv4_hdr *hdr, struct pico_frame *f,
							uint8_t proto);

#endif
