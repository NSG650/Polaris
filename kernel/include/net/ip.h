#ifndef IP_H
#define IP_H

#include <net/net.h>
#include <stddef.h>
#include <stdint.h>

// I hate big endian :D
struct ip_packet {
	uint8_t internet_header_length : 4;
	uint8_t version : 4;
	uint8_t type_of_service;
	uint16_t length;
	uint16_t id;
	uint16_t fragment_offset;
	uint8_t time_to_live;
	uint8_t protocol;
	uint16_t checksum;
	uint8_t source_protocol_addr[4];
	uint8_t destination_protocol_addr[4];
	uint8_t data[];
} __attribute__((packed));

uint16_t ip_calculate_checksum(void *addr, int count);
void ip_send(struct ip_packet *packet, uint16_t length,
			 uint8_t *destination_protocol_addr, uint8_t *dest_mac,
			 struct net_nic_interfaces *nic_interfaces);
void ip_handle(struct ip_packet *packet, uint32_t length, uint8_t *dest_mac,
			   struct net_nic_interfaces *nic_interfaces);

#endif
