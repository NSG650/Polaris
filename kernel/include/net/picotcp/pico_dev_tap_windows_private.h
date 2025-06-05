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

/*********************************************************************
   NOTES: This is the Windows-only driver, a Linux-equivalent is available, too
		  You need to have an OpenVPN TUN/TAP network adapter installed, first
		  This driver is barely working:
 * Only TAP-mode is supported (TUN is not)
 * it will simply open the first TAP device it can find
 * there is memory being allocated that's never freed
 * there is no destroy function, yet
 * it has only been tested on a Windows 7 machine
 *********************************************************************/

#ifndef __PICO_DEV_TAP_WINDOWS_PRIVATE_H
#define __PICO_DEV_TAP_WINDOWS_PRIVATE_H

/* Extra defines (vnz) */
#define TAP_WIN_COMPONENT_ID "tap0901"
#define TAP_WIN_MIN_MAJOR 9
#define TAP_WIN_MIN_MINOR 9
#define PACKAGE_NAME "PicoTCP WinTAP"

/* Extra structs */
struct tap_reg {
	const char *guid;
	struct tap_reg *next;
};

struct panel_reg {
	const char *name;
	const char *guid;
	struct panel_reg *next;
};

/*
 * =============
 * TAP IOCTLs
 * =============
 */

#define TAP_WIN_CONTROL_CODE(request, method) \
	CTL_CODE(FILE_DEVICE_UNKNOWN, request, method, FILE_ANY_ACCESS)

/* Present in 8.1 */

#define TAP_WIN_IOCTL_GET_MAC TAP_WIN_CONTROL_CODE(1, METHOD_BUFFERED)
#define TAP_WIN_IOCTL_GET_VERSION TAP_WIN_CONTROL_CODE(2, METHOD_BUFFERED)
#define TAP_WIN_IOCTL_GET_MTU TAP_WIN_CONTROL_CODE(3, METHOD_BUFFERED)
#define TAP_WIN_IOCTL_GET_INFO TAP_WIN_CONTROL_CODE(4, METHOD_BUFFERED)
#define TAP_WIN_IOCTL_CONFIG_POINT_TO_POINT \
	TAP_WIN_CONTROL_CODE(5, METHOD_BUFFERED)
#define TAP_WIN_IOCTL_SET_MEDIA_STATUS TAP_WIN_CONTROL_CODE(6, METHOD_BUFFERED)
#define TAP_WIN_IOCTL_CONFIG_DHCP_MASQ TAP_WIN_CONTROL_CODE(7, METHOD_BUFFERED)
#define TAP_WIN_IOCTL_GET_LOG_LINE TAP_WIN_CONTROL_CODE(8, METHOD_BUFFERED)
#define TAP_WIN_IOCTL_CONFIG_DHCP_SET_OPT \
	TAP_WIN_CONTROL_CODE(9, METHOD_BUFFERED)

/* Added in 8.2 */

/* obsoletes TAP_WIN_IOCTL_CONFIG_POINT_TO_POINT */
#define TAP_WIN_IOCTL_CONFIG_TUN TAP_WIN_CONTROL_CODE(10, METHOD_BUFFERED)

/*
 * =================
 * Registry keys
 * =================
 */

#define ADAPTER_KEY                                                        \
	"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-" \
	"08002BE10318}"

#define NETWORK_CONNECTIONS_KEY                                              \
	"SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-" \
	"08002BE10318}"

/*
 * ======================
 * Filesystem prefixes
 * ======================
 */

#define USERMODEDEVICEDIR "\\\\.\\Global\\"
#define SYSDEVICEDIR "\\Device\\"
#define USERDEVICEDIR "\\DosDevices\\Global\\"
#define TAP_WIN_SUFFIX ".tap"

#endif
