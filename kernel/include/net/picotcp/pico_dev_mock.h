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
#ifndef INCLUDE_PICO_MOCK
#define INCLUDE_PICO_MOCK
#include "pico_config.h"
#include "pico_device.h"

struct mock_frame {
	uint8_t *buffer;
	int len;
	int read;

	struct mock_frame *next;
};

struct mock_device {
	struct pico_device *dev;
	struct mock_frame *in_head;
	struct mock_frame *in_tail;
	struct mock_frame *out_head;
	struct mock_frame *out_tail;

	uint8_t *mac;
};

struct mock_device;
/* A mockup-device for the purpose of testing. It provides a couple of extra
 * "network"-functions, which represent the network-side of the device. A
 * network_send will result in mock_poll reading something, a network_read will
 * see if the stack has sent anything through our mock-device. */
void pico_mock_destroy(struct pico_device *dev);
struct mock_device *pico_mock_create(struct pico_stack *S, uint8_t *mac);

int pico_mock_network_read(struct mock_device *mock, void *buf, int len);
int pico_mock_network_write(struct mock_device *mock, const void *buf, int len);

/* TODO */
/* we could use a few checking functions, e.g. one to see if it's a valid IP
 * packet, if it's TCP, if the IP-address matches,... */
/* That would be useful to avoid having to manually create buffers of what you
 * expect, probably with masks for things that are random,... */
uint32_t mock_get_sender_ip4(struct mock_device *mock, const char *buf,
							 int len);

int mock_ip_protocol(struct mock_device *mock, const char *buf, int len);
int mock_icmp_type(struct mock_device *mock, const char *buf, int len);
int mock_icmp_code(struct mock_device *mock, const char *buf, int len);
#endif
