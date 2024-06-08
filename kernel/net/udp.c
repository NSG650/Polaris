#include <debug/debug.h>
#include <fs/devtmpfs.h>
#include <fs/vfs.h>
#include <mm/slab.h>
#include <net/net.h>
#include <net/udp.h>

uint16_t udp_calculate_checksum(void *addr, int count) {
	return ip_calculate_checksum(addr, count);
}

void udp_handle(struct ip_packet *packet, uint32_t length, uint8_t *dest_mac,
				struct net_nic_interfaces *nic_interfaces) {
	struct udp_packet *udp_pack = (struct udp_packet *)packet->data;
	if (BSWAP16(udp_pack->destination_port) == 7) {
		uint16_t temp = udp_pack->destination_port;
		udp_pack->destination_port = udp_pack->source_port;
		udp_pack->source_port = temp;
		ip_send(packet, length, packet->source_protocol_addr, dest_mac,
				nic_interfaces);
	}
}

void udp_send(struct ip_packet *packet, uint16_t length,
			  uint8_t *destination_protocol_addr, uint8_t *dest_mac,
			  uint16_t dest_port, uint16_t source_port,
			  struct net_nic_interfaces *nic_interfaces) {
	struct udp_packet *udp_pack = (struct udp_packet *)packet->data;
	udp_pack->source_port = BSWAP16(source_port);
	udp_pack->destination_port = BSWAP16(dest_port);
	udp_pack->length = BSWAP16(length + sizeof(struct udp_packet));
	udp_pack->checksum = 0; // turns out incorrectly calculating the checksum
							// leads it to refusing it

	packet->protocol = 17;
	ip_send(packet,
			length + sizeof(struct udp_packet) + sizeof(struct ip_packet),
			destination_protocol_addr, dest_mac, nic_interfaces);
}
