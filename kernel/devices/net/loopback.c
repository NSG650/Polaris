#include <devices/net/loopback.h>
#include <klibc/mem.h>
#include <mm/slab.h>

struct net_nic_interfaces nic_loopback = {.get_mac_addr = loopback_get_mac_addr,
										  .send_packet = loopback_send_packet};

uint8_t *loopback_get_mac_addr(void) {
	uint8_t mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	return mac;
}

void loopback_send_packet(uint8_t *dest, void *packet, uint32_t packet_length,
						  uint16_t protocol) {
	(void)dest;

	struct network_packet *data =
		kmalloc(packet_length + sizeof(struct network_packet));
	memzero(data, sizeof(struct network_packet));

	data->type = BSWAP16(protocol);
	memcpy(data->data, packet, packet_length);

	net_handle_packet(data, packet_length + sizeof(struct network_packet),
					  &nic_loopback);
	kfree(data);
}
