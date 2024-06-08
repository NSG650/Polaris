#include <debug/debug.h>
#include <klibc/mem.h>
#include <mm/slab.h>
#include <net/icmp.h>
#include <net/ip.h>
#include <net/net.h>

void icmp_echo_reply(struct ip_packet *ip_pack, uint32_t length,
					 uint8_t *dest_mac,
					 struct net_nic_interfaces *nic_interfaces) {
	uint8_t source_protocol_addr[4] = {0};
	memcpy(source_protocol_addr, ip_pack->source_protocol_addr, 4);

	struct icmp_header *icmp_pack = (struct icmp_header *)ip_pack->data;

	if (icmp_pack->type != 8) {
		return;
	}

	icmp_pack->type = 0;
	icmp_pack->checksum = 0;
	icmp_pack->checksum = ip_calculate_checksum(
		icmp_pack, length - ip_pack->internet_header_length * 4);

	ip_send(ip_pack, length, source_protocol_addr, dest_mac, nic_interfaces);
}
