#ifndef UDP_H
#define UDP_H

#include <net/ip.h>
#include <stddef.h>
#include <stdint.h>

struct udp_packet {
	uint16_t source_port;
	uint16_t destination_port;
	uint16_t length;
	uint16_t checksum;
	uint8_t data[];
} __attribute__((packed));

uint16_t udp_calculate_checksum(void *addr, int count);
void udp_handle(struct ip_packet *packet, uint32_t length, uint8_t *dest_mac,
				struct net_nic_interfaces *nic_interfaces);
void udp_send(struct ip_packet *packet, uint16_t length,
			  uint8_t *destination_protocol_addr, uint8_t *dest_mac,
			  uint16_t dest_port, uint16_t source_port,
			  struct net_nic_interfaces *nic_interfaces);

#endif
