#ifndef ICMP_H
#define ICMP_H

#include <stddef.h>
#include <stdint.h>

#include <net/ip.h>

struct icmp_header {
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint8_t data[];
} __attribute__((packed));

void icmp_echo_reply(struct ip_packet *ip_pack, uint32_t length,
					 uint8_t *dest_mac,
					 struct net_nic_interfaces *nic_interfaces);

#endif
