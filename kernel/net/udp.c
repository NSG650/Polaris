#include <debug/debug.h>
#include <devices/termios.h>
#include <fs/devtmpfs.h>
#include <fs/vfs.h>
#include <mm/slab.h>
#include <net/net.h>
#include <net/udp.h>
#include <net/udp_shell.h>

uint16_t udp_calculate_checksum(void *addr, int count) {
	return ip_calculate_checksum(addr, count);
}

void udp_handle(struct ip_packet *packet, uint32_t length, uint8_t *dest_mac) {
	struct udp_packet *udp_pack = (struct udp_packet *)packet->data;
	if (BSWAP16(udp_pack->destination_port) == 7) {
		uint16_t temp = udp_pack->destination_port;
		udp_pack->destination_port = udp_pack->source_port;
		udp_pack->source_port = temp;
		ip_send(packet, length, packet->source_protocol_addr, dest_mac);
	} else if (BSWAP16(udp_pack->destination_port) == 1337) {
		struct udp_shell_data *shell_data =
			kmalloc(sizeof(struct udp_shell_data));
		shell_data->buffer =
			kmalloc((udp_pack->length - sizeof(struct udp_packet)) + 1);
		memcpy(shell_data->buffer, udp_pack->data,
			   udp_pack->length - sizeof(struct udp_packet));
		shell_data->buffer[udp_pack->length - sizeof(struct udp_packet)] = '\0';
		memcpy(shell_data->ip, packet->source_protocol_addr, 4);
		memcpy(shell_data->mac, dest_mac, 6);
		shell_data->source_port = udp_pack->source_port;
		udp_shell_entry_add(dest_mac, shell_data);
	}
}

void udp_send(struct ip_packet *packet, uint16_t length,
			  uint8_t *destination_protocol_addr, uint8_t *dest_mac,
			  uint16_t dest_port, uint16_t source_port) {
	struct udp_packet *udp_pack = (struct udp_packet *)packet->data;
	udp_pack->source_port = BSWAP16(source_port);
	udp_pack->destination_port = BSWAP16(dest_port);
	udp_pack->length = BSWAP16(length + sizeof(struct udp_packet));
	udp_pack->checksum = 0; // turns out incorrectly calculating the checksum
							// leads it to refusing it

	packet->protocol = 17;
	ip_send(packet,
			length + sizeof(struct udp_packet) + sizeof(struct ip_packet),
			destination_protocol_addr, dest_mac);
}
