#include <debug/debug.h>
#include <net/net.h>

void net_handle_packet(uint8_t *packet, uint16_t packet_length) {
	struct network_packet *net_pack = (struct network_packet *)packet;

	uint8_t *data = packet + sizeof(struct network_packet);
	size_t data_length = packet_length - sizeof(struct network_packet);

	kprintf("NET: net_pack->destination_mac %x:%x:%x:%x:%x:%x\n",
			net_pack->destination_mac[0], net_pack->destination_mac[1],
			net_pack->destination_mac[2], net_pack->destination_mac[3],
			net_pack->destination_mac[4], net_pack->destination_mac[5]);

	kprintf("NET: net_pack->source_mac %x:%x:%x:%x:%x:%x\n",
			net_pack->source_mac[0], net_pack->source_mac[1],
			net_pack->source_mac[2], net_pack->source_mac[3],
			net_pack->source_mac[4], net_pack->source_mac[5]);

	if (BSWAP16(net_pack->type) == REQ_TYPE_ARP) {
		kprintf("NET: Got an ARP packet\n");
	}

	else if (BSWAP16(net_pack->type) == REQ_TYPE_IP) {
		kprintf("NET: Got an IP packet\n");
	}

	else {
		kprintf("NET: Unknown request type 0x%x\n", BSWAP16(net_pack->type));
	}
}