#include <debug/debug.h>
#include <net/arp.h>
#include <net/ip.h>
#include <net/net.h>
#include <net/net_sock.h>
#include <sched/sched.h>
#include <sys/prcb.h>

#include "lwip/api.h"
#include "lwip/init.h"
#include "lwip/sockets.h"
#include "lwip/tcpip.h"

net_socket_vec_t net_sockets_table = {0};
lock_t net_sockets_table_lock = {0};

void net_sockets_polling_thread(void) {
	for (;;) {
		if (spinlock_acquire(&net_sockets_table_lock)) {
			fd_set readfds, writefds;
			FD_ZERO(&readfds);
			FD_ZERO(&writefds);
			int max_fd = -1;

			for (int i = 0; i < net_sockets_table.length; i++) {
				struct net_socket *socket = net_sockets_table.data[i];
				if (socket->lwip_fd >= 0) {
					FD_SET(socket->lwip_fd, &readfds);
					FD_SET(socket->lwip_fd, &writefds);
					if (socket->lwip_fd > max_fd)
						max_fd = socket->lwip_fd;
				}
			}

			if (max_fd >= 0) {
				struct timeval tv = {.tv_sec = 0, .tv_usec = 10};
				lwip_select(max_fd + 1, &readfds, &writefds, NULL, &tv);
				for (int i = 0; i < net_sockets_table.length; i++) {
					struct net_socket *socket = net_sockets_table.data[i];
					if (socket->lwip_fd < 0)
						continue;
					if (FD_ISSET(socket->lwip_fd, &readfds)) {
						socket->sock.res.status |= POLLIN;
						event_trigger(&socket->sock.res.event, false);
					}
					if (FD_ISSET(socket->lwip_fd, &writefds)) {
						socket->sock.res.status |= POLLOUT;
						event_trigger(&socket->sock.res.event, false);
					}
				}
			}
			spinlock_drop(&net_sockets_table_lock);
		}
		sched_yield(true);
	}
}

void net_handle_packet_thread(uint64_t *handover) {
	if (handover == NULL) {
		return;
	}
	uint8_t *packet = (uint8_t *)handover[0];
	uint16_t length = (uint16_t)handover[1];
	struct net_nic_interfaces *nic_interface =
		(struct net_nic_interfaces *)handover[2];

	struct pbuf *p = pbuf_alloc(PBUF_RAW, length, PBUF_RAM);
	if (p == NULL) {
		kfree(packet);
		kfree(handover);
		thread_kill(sched_get_running_thread(), true);
	}

	void *targ = (void *)p->payload;
	memcpy(targ, packet, length);
	nic_interface->lwip.input(p, &nic_interface->lwip);

	// net_handle_packet(packet, length, nic_interface);
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

void lwip_init_callback(void *arg) {
	(void)arg;
	kprintf("NET: Running LwIP " LWIP_VERSION_STRING "\n");
}

void net_init(void) {
	arp_init();
	tcpip_init(lwip_init_callback, NULL);
	vec_init(&net_sockets_table);
	thread_create((uintptr_t)net_sockets_polling_thread, 0, false, kernel_proc);
}
