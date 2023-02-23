#include <debug/debug.h>
#include <klibc/mem.h>
#include <mm/slab.h>
#include <net/icmp.h>
#include <net/ip.h>
#include <net/net.h>
#include <sched/sched.h>
#include <sys/prcb.h>

void icmp_echo_reply(uint64_t *handover) {
	struct ip_packet *ip_pack = (struct ip_packet *)handover[0];
	uint32_t length = (uint32_t)handover[1];
	uint8_t *dest_mac = (uint8_t *)handover[2];
	uint8_t source_protocol_addr[4] = {0};
	memcpy(source_protocol_addr, ip_pack->source_protocol_addr, 4);

	struct icmp_header *icmp_pack = (struct icmp_header *)ip_pack->data;

	kprintf("ICMP: Got type: %d\n", icmp_pack->type);

	if (icmp_pack->type != 8) {
		kfree(ip_pack);
		kfree(dest_mac);
		kfree(handover);
		thread_kill(prcb_return_current_cpu()->running_thread, true);
	}

	icmp_pack->type = 0;
	icmp_pack->checksum = 0;
	icmp_pack->checksum =
		ip_calculate_checksum(icmp_pack, length - sizeof(struct ip_packet));
	ip_send(ip_pack, length, source_protocol_addr, dest_mac);

	kfree(ip_pack);
	kfree(dest_mac);
	kfree(handover);

	thread_kill(prcb_return_current_cpu()->running_thread, true);
}