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
#ifndef PICO_TFTP_H
#define PICO_TFTP_H

#include "pico_addressing.h"
#include <stddef.h>
#include <stdint.h>

#define PICO_TFTP_PORT (69)
#define PICO_TFTP_PAYLOAD_SIZE (512)

#define PICO_TFTP_NONE 0
#define PICO_TFTP_RRQ 1
#define PICO_TFTP_WRQ 2
#define PICO_TFTP_DATA 3
#define PICO_TFTP_ACK 4
#define PICO_TFTP_ERROR 5
#define PICO_TFTP_OACK 6

/* Callback user events */
#define PICO_TFTP_EV_OK 0
#define PICO_TFTP_EV_OPT 1
#define PICO_TFTP_EV_ERR_PEER 2
#define PICO_TFTP_EV_ERR_LOCAL 3

/* TFTP ERROR CODES */
#define TFTP_ERR_UNDEF 0
#define TFTP_ERR_ENOENT 1
#define TFTP_ERR_EACC 2
#define TFTP_ERR_EXCEEDED 3
#define TFTP_ERR_EILL 4
#define TFTP_ERR_ETID 5
#define TFTP_ERR_EEXIST 6
#define TFTP_ERR_EUSR 7
#define TFTP_ERR_EOPT 8

/* Session options */
#define PICO_TFTP_OPTION_FILE 1

/* timeout: 0 -> adaptative, 1-255 -> fixed */
#define PICO_TFTP_OPTION_TIME 2

#define PICO_TFTP_MAX_TIMEOUT 255
#define PICO_TFTP_MAX_FILESIZE (65535 * 512 - 1)

/* MAX_OPTIONS_SIZE: "timeout" 255 "tsize" filesize =>  8 + 4 + 6 + 11 */
#define MAX_OPTIONS_SIZE 29

/* RRQ and WRQ packets (opcodes 1 and 2 respectively) */
PACKED_STRUCT_DEF pico_tftp_hdr {
	uint16_t opcode;
};

/* DATA or ACK (opcodes 3 and 4 respectively)*/
PACKED_STRUCT_DEF pico_tftp_data_hdr {
	uint16_t opcode;
	uint16_t block;
};

/* ERROR (opcode 5) */
PACKED_STRUCT_DEF pico_tftp_err_hdr {
	uint16_t opcode;
	uint16_t error_code;
};
#define PICO_TFTP_TOTAL_BLOCK_SIZE \
	(PICO_TFTP_PAYLOAD_SIZE + (int32_t)sizeof(struct pico_tftp_data_hdr))
#define tftp_payload(p) (((uint8_t *)(p)) + sizeof(struct pico_tftp_data_hdr))

struct pico_tftp_session;

struct pico_tftp_server_t {
	void (*listen_callback)(union pico_address *addr, uint16_t port,
							uint16_t opcode, char *filename, int32_t len);
	struct pico_socket *listen_socket;
	uint8_t tftp_block[PICO_TFTP_TOTAL_BLOCK_SIZE];
};

struct pico_tftp_session *pico_tftp_session_setup(struct pico_stack *S,
												  union pico_address *a,
												  uint16_t family);
int pico_tftp_set_option(struct pico_tftp_session *session, uint8_t type,
						 int32_t value);
int pico_tftp_get_option(struct pico_tftp_session *session, uint8_t type,
						 int32_t *value);

int pico_tftp_start_rx(struct pico_stack *S, struct pico_tftp_session *session,
					   uint16_t port, const char *filename,
					   int (*user_cb)(struct pico_tftp_session *session,
									  uint16_t event, uint8_t *block,
									  int32_t len, void *arg),
					   void *arg);
int pico_tftp_start_tx(struct pico_stack *S, struct pico_tftp_session *session,
					   uint16_t port, const char *filename,
					   int (*user_cb)(struct pico_tftp_session *session,
									  uint16_t event, uint8_t *block,
									  int32_t len, void *arg),
					   void *arg);

int pico_tftp_reject_request(struct pico_stack *S, union pico_address *addr,
							 uint16_t port, uint16_t error_code,
							 const char *error_message);
int32_t pico_tftp_send(struct pico_tftp_session *session, const uint8_t *data,
					   int32_t len);

int pico_tftp_listen(struct pico_stack *S, uint16_t family,
					 void (*cb)(union pico_address *addr, uint16_t port,
								uint16_t opcode, char *filename, int32_t len));

int pico_tftp_parse_request_args(char *args, int32_t len, int *options,
								 uint8_t *timeout, int32_t *filesize);

int pico_tftp_abort(struct pico_tftp_session *session, uint16_t error,
					const char *reason);
int pico_tftp_close_server(struct pico_stack *S);

int pico_tftp_get_file_size(struct pico_tftp_session *session,
							int32_t *file_size);

/* SPECIFIC APPLICATION DRIVEN FUNCTIONS */
struct pico_tftp_session *pico_tftp_app_setup(struct pico_stack *S,
											  union pico_address *a,
											  uint16_t port, uint16_t family,
											  int *synchro);

int pico_tftp_app_start_rx(struct pico_stack *S,
						   struct pico_tftp_session *session,
						   const char *filename);
int pico_tftp_app_start_tx(struct pico_stack *S,
						   struct pico_tftp_session *session,
						   const char *filename);

int32_t pico_tftp_get(struct pico_tftp_session *session, uint8_t *data,
					  int32_t len);
int32_t pico_tftp_put(struct pico_tftp_session *session, uint8_t *data,
					  int32_t len);

#endif
