#ifndef ARP_H
#define ARP_H

#include <klibc/vec.h>
#include <net/net.h>
#include <stddef.h>
#include <stdint.h>

struct arp_table_entry {
	uint8_t ip_addr[4];
	uint8_t mac_addr[6];
};

struct arp_packet {
	uint16_t hw_type;
	uint16_t protocol;
	uint8_t hw_addr_len;
	uint8_t protocol_addr_len;
	uint16_t opcode;
	uint8_t source_mac[6];
	uint8_t source_protocol_addr[4];
	uint8_t destination_mac[6];
	uint8_t destination_protocol_addr[4];
};

typedef vec_t(struct arp_table_entry *) arp_table_vec_t;

void arp_init(void);
struct arp_table_entry *arp_get_table_entry(uint8_t *ip_address);
void arp_handle(struct arp_packet *packet, uint32_t length,
				struct net_nic_interfaces *nic_interfaces);
void arp_send(struct arp_packet *packet, uint32_t length,
			  struct net_nic_interfaces *nic_interfaces);
void arp_lookup(uint8_t *ip, struct net_nic_interfaces *nic_interfaces);

#endif
