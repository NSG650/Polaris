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
#ifndef INCLUDE_PICO_PPP
#define INCLUDE_PICO_PPP

#include "pico_config.h"
#include "pico_device.h"

void pico_ppp_destroy(struct pico_device *ppp);
struct pico_device *pico_ppp_create(struct pico_stack *S);

int pico_ppp_connect(struct pico_device *dev);
int pico_ppp_disconnect(struct pico_device *dev);

int pico_ppp_set_serial_read(struct pico_device *dev,
							 int (*sread)(struct pico_device *, void *, int));
int pico_ppp_set_serial_write(struct pico_device *dev,
							  int (*swrite)(struct pico_device *, const void *,
											int));
int pico_ppp_set_serial_set_speed(struct pico_device *dev,
								  int (*sspeed)(struct pico_device *,
												uint32_t));

int pico_ppp_set_apn(struct pico_device *dev, const char *apn);
int pico_ppp_set_username(struct pico_device *dev, const char *username);
int pico_ppp_set_password(struct pico_device *dev, const char *password);

#endif /* INCLUDE_PICO_PPP */
