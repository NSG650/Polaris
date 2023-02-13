#ifndef NET_H
#define NET_H

#include <stddef.h>
#include <stdint.h>

#define BSWAP16(x) ((((x)&0xff) << 8) | (((x)&0xff00) >> 8))

#define BSWAP32(x)                                        \
	((((x)&0x000000ff) << 24) | (((x)&0x0000ff00) << 8) | \
	 (((x)&0x00ff0000) >> 8) | (((x)&0xff000000) >> 24));

#define REQ_TYPE_ARP 0x0806
#define REQ_TYPE_IP 0x0800

struct network_packet {
	uint8_t destination_mac[6];
	uint8_t source_mac[6];
	uint16_t type;
	uint8_t data[];
} __attribute__((packed));

void net_handle_packet(uint8_t *packet, uint16_t packet_length);

#endif