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
#ifndef INCLUDE_PICO_VDE
#define INCLUDE_PICO_VDE
#include "pico_config.h"
#include "pico_device.h"
#include <libvdeplug.h>

void pico_vde_destroy(struct pico_device *vde);
struct pico_device *pico_vde_create(struct pico_stack *S, char *sock,
									char *name, uint8_t *mac);
void pico_vde_set_packetloss(struct pico_device *dev, uint32_t in_pct,
							 uint32_t out_pct);

#ifdef PICO_SUPPORT_TICKLESS
int pico_vde_WFI(struct pico_device *dev, int timeout_ms);
void pico_vde_dsr(void *arg);
#endif

#endif
