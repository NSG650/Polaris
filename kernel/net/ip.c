#include <debug/debug.h>
#include <klibc/mem.h>
#include <mm/slab.h>
#include <net/icmp.h>
#include <net/ip.h>
#include <net/net.h>
#include <sched/sched.h>

extern uint8_t my_ip[];

uint16_t ip_calculate_checksum(void *addr, int count) {
	// Taken from https://tools.ietf.org/html/rfc1071

	register uint32_t sum = 0;
	uint16_t *ptr = addr;

	while (count > 1) {
		/*  This is the inner loop */
		sum += *ptr++;
		count -= 2;
	}

	/*  Add left-over byte, if any */
	if (count > 0)
		sum += *(uint8_t *)ptr;

	/*  Fold 32-bit sum to 16 bits */
	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	return ~sum;
}

void ip_send(struct ip_packet *packet, uint32_t length,
			 uint8_t *destination_protocol_addr, uint8_t *dest_mac) {

	packet->version = 4;
	packet->internet_header_length = 5;
	packet->length = BSWAP16(length);
	packet->id = BSWAP16(packet->id);
	packet->fragment_offset = BSWAP16(0x4000);
	packet->time_to_live = 64;

	packet->checksum = 0;

	memcpy(packet->destination_protocol_addr, destination_protocol_addr, 4);
	// hard coding the ip yet again

	memcpy(packet->source_protocol_addr, my_ip, 4);

	packet->checksum = ip_calculate_checksum(packet, sizeof(struct ip_packet));

	net_send_packet(dest_mac, packet, length, REQ_TYPE_IP);
}

void ip_handle(struct ip_packet *packet, uint32_t length, uint8_t *dest_mac) {
	kprintf("IP: Got packet from %d.%d.%d.%d\n",
			packet->source_protocol_addr[0], packet->source_protocol_addr[1],
			packet->source_protocol_addr[2], packet->source_protocol_addr[3]);

	if (packet->protocol == 1) {
		void *clone = kmalloc(length);
		memcpy(clone, packet, length);
		uint8_t *clone_mac = kmalloc(6);
		memcpy(clone_mac, dest_mac, 6);

		icmp_echo_reply(clone, length, dest_mac);
	} else {
		kprintf("IP: Unknown protocol %d\n", packet->protocol);
	}
}