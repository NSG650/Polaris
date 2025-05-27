#include <debug/debug.h>
#include <net/arp.h>
#include <net/ip.h>
#include <net/net.h>
#include <sched/sched.h>
#include <sys/prcb.h>

void net_handle_packet_thread(uint64_t *handover) {
	if (handover == NULL) {
		return;
	}
	uint8_t *packet = (uint8_t *)handover[0];
	uint16_t length = (uint16_t)handover[1];
	struct net_nic_interfaces *nic_interfaces =
		(struct net_nic_interfaces *)handover[2];
	net_handle_packet(packet, length, nic_interfaces);
	kfree(packet);
	kfree(handover);
	thread_kill(sched_get_running_thread(), true);
}

void net_handle_packet(void *packet, uint16_t packet_length,
					   struct net_nic_interfaces *nic_interfaces) {
	struct network_packet *net_pack = (struct network_packet *)packet;

	if (net_pack == NULL) {
		return;
	}

	void *data = packet + sizeof(struct network_packet);
	size_t data_length = packet_length - sizeof(struct network_packet);

	if (BSWAP16(net_pack->type) == REQ_TYPE_ARP) {
		struct arp_packet *arp_pack = (struct arp_packet *)data;
		arp_handle(arp_pack, data_length, nic_interfaces);
	}

	else if (BSWAP16(net_pack->type) == REQ_TYPE_IP) {
		struct ip_packet *ip_pack = (struct ip_packet *)data;
		ip_handle(ip_pack, data_length, net_pack->source_mac, nic_interfaces);
	}

	else {
		kprintf("NET: Unknown request type 0x%x\n", BSWAP16(net_pack->type));
	}
}

void net_init(void) {
	arp_init();
}
