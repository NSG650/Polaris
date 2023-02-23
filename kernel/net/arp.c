#include <asm/asm.h>
#include <debug/debug.h>
#include <net/arp.h>
#include <net/net.h>

arp_table_vec_t arp_table;

uint8_t broadcast_ip[4] = {0xff, 0xff, 0xff, 0xff};
uint8_t broadcast_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

uint8_t last_ip[4] = {0x00, 0x00, 0x00, 0x00};

void arp_init(void) {
	vec_init(&arp_table);
}

struct arp_table_entry *arp_get_table_entry(uint8_t *ip_address) {
	for (int i = 0; i < arp_table.length; i++) {
		if (memcmp(arp_table.data[i]->ip_addr, ip_address, 4) == 0)
			return arp_table.data[i];
	}
	return NULL;
}

void arp_handle(struct arp_packet *packet, uint32_t length) {
	uint8_t dest_mac[6];
	uint8_t dest_protocol_addr[4];

	memcpy(dest_mac, packet->source_mac, 6);
	memcpy(dest_protocol_addr, packet->destination_protocol_addr, 4);

	if (BSWAP16(packet->opcode) == 1) { // ARP_REQUEST
		kprintf("ARP: Got an ARP_REQUEST from %d.%d.%d.%d\n",
				dest_protocol_addr[0], dest_protocol_addr[1],
				dest_protocol_addr[2], dest_protocol_addr[3]);
		memcpy(packet->source_mac, net_get_mac_addr(), 6);

		// Hard code the IP :))
		packet->source_protocol_addr[0] = 192;
		packet->source_protocol_addr[1] = 168;
		packet->source_protocol_addr[2] = 1;
		packet->source_protocol_addr[3] = 35;

		memcpy(packet->destination_protocol_addr, dest_protocol_addr, 4);
		memcpy(packet->destination_mac, dest_mac, 6);

		packet->opcode = BSWAP16(2); // ARP_REPLY

		packet->hw_addr_len = 6;
		packet->protocol_addr_len = 4;

		packet->hw_type = BSWAP16(1); // HW_TYPE_ETHERNET

		packet->protocol = BSWAP16(REQ_TYPE_IP);

		net_send_packet(packet->destination_mac, packet,
						sizeof(struct arp_packet), REQ_TYPE_ARP);
	}

	else if (BSWAP16(packet->opcode) == 2) {
		kprintf("ARP: We got a reply\n");
		kprintf("ARP: Got Mac Addr %x:%x:%x:%x:%x:%x\n", dest_mac[0],
				dest_mac[1], dest_mac[2], dest_mac[3], dest_mac[4],
				dest_mac[5]);
		uint8_t our_ip[4] = {192, 168, 1, 35};
		if (memcmp(dest_protocol_addr, our_ip, 4))
			return;
		struct arp_table_entry *entry = kmalloc(sizeof(struct arp_table_entry));
		memcpy(entry->ip_addr, last_ip, 4);
		memcpy(entry->mac_addr, dest_mac, 6);
		vec_push(&arp_table, entry);
		memset(last_ip, 0, 4);
	}
}

void arp_send(struct arp_packet *packet, uint32_t length) {
	memcpy(packet->source_mac, net_get_mac_addr(), 6);

	// Hard code the IP :))
	packet->source_protocol_addr[0] = 192;
	packet->source_protocol_addr[1] = 168;
	packet->source_protocol_addr[2] = 1;
	packet->source_protocol_addr[3] = 35;

	packet->opcode = BSWAP16(1);

	packet->hw_addr_len = 6;
	packet->protocol_addr_len = 4;

	packet->hw_type = BSWAP16(1); // HW_TYPE_ETHERNET

	packet->protocol = BSWAP16(REQ_TYPE_IP);

	net_send_packet(packet->destination_mac, packet, length, REQ_TYPE_ARP);
}

void arp_lookup(uint8_t *ip) {
	struct arp_packet *lookup_packet = kmalloc(sizeof(struct arp_packet));
	memcpy(last_ip, ip, 4);
	memcpy(lookup_packet->destination_protocol_addr, ip, 4);
	memset(lookup_packet->destination_mac, 0xff, 6);
	arp_send(lookup_packet, sizeof(struct arp_packet));
}