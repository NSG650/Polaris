#include <debug/debug.h>
#include <net/arp.h>
#include <net/net.h>

void net_handle_packet(uint8_t *packet, uint16_t packet_length) {
	struct network_packet *net_pack = (struct network_packet *)packet;

	uint8_t *data = packet + sizeof(struct network_packet);
	size_t data_length = packet_length - sizeof(struct network_packet);

	if (BSWAP16(net_pack->type) == REQ_TYPE_ARP) {
		kprintf("NET: Got an ARP packet\n");
		struct arp_packet *arp_pack = (struct arp_packet *)data;
		arp_handle(arp_pack, data_length);
	}

	else if (BSWAP16(net_pack->type) == REQ_TYPE_IP) {
		kprintf("NET: Got an IP packet\n");
	}

	else {
		kprintf("NET: Unknown request type 0x%x\n", BSWAP16(net_pack->type));
	}
}

void net_init(void) {
	arp_init();
}