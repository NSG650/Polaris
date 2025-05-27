#ifndef LOOPBACK_H
#define LOOPBACK_H

#include <net/net.h>

uint8_t *loopback_get_mac_addr(void);
void loopback_send_packet(uint8_t *dest, void *packet, uint32_t packet_length,
						  uint16_t protocol);

extern struct net_nic_interfaces nic_loopback;

#endif
