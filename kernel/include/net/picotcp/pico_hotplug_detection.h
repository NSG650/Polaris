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
#ifndef INCLUDE_PICO_SUPPORT_HOTPLUG
#define INCLUDE_PICO_SUPPORT_HOTPLUG
#include "pico_stack.h"

#define PICO_HOTPLUG_EVENT_UP 1	  /* link went up */
#define PICO_HOTPLUG_EVENT_DOWN 2 /* link went down */

#define PICO_HOTPLUG_INTERVAL 100

int pico_hotplug_dev_cmp(void *ka, void *kb);
/* register your callback to be notified of hotplug events on a certain device.
 * Note that each callback will be called at least once, shortly after adding,
 * for initialization.
 */
int pico_hotplug_register(struct pico_device *dev,
						  void (*cb)(struct pico_device *dev, int event));
int pico_hotplug_deregister(struct pico_device *dev,
							void (*cb)(struct pico_device *dev, int event));

#endif /* _INCLUDE_PICO_SUPPORT_HOTPLUG */
