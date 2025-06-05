/*********************************************************************
 * PicoTCP-NG
 * Copyright (c) 2020 Daniele Lacamera <root@danielinux.net>
 *
 * This file also includes code from:
 * PicoTCP
 * Copyright (c) 2012-2017 Altran Intelligent Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
 *
 * PicoTCP-NG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) version 3.
 *
 * PicoTCP-NG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 *
 *
 *********************************************************************/
#include "heap.h"
#include "pico_config.h"
#include "pico_constants.h"
#include "pico_frame.h"
#include "pico_queue.h"
#ifndef INCLUDE_PICO_STACK
#define INCLUDE_PICO_STACK
#include "pico_arp.h"
#include "pico_dhcp_client.h"
#include "pico_dhcp_common.h"
#include "pico_mdns.h"
#include "pico_olsr.h"
#include "pico_sntp_client.h"
#include "pico_tftp.h"

#ifdef PICO_SUPPORT_IPV6PMTU
#include "pico_ipv6_pmtu.h"
#endif
#ifdef PICO_SUPPORT_DHCPC
#include "pico_dhcp_client.h"
#endif

#ifdef PICO_SUPPORT_PACKET_SOCKETS
#include "pico_socket_ll.h"
#endif

struct arp_service_ipconflict {
	struct pico_eth mac;
	struct pico_ip4 ip;
	void (*conflict)(struct pico_stack *, int);
};

#define PROTO_DEF_NR 11
#define PROTO_DEF_AVG_NR 4
#define PROTO_DEF_SCORE 32
#define PROTO_MIN_SCORE 32
#define PROTO_MAX_SCORE 128
#define PROTO_LAT_IND                                                          \
	3 /* latency indication 0-3 (lower is better latency performance), x1, x2, \
		 x4, x8 */
#define PROTO_MAX_LOOP                                                       \
	(PROTO_MAX_SCORE << PROTO_LAT_IND) /* max global loop score, so per tick \
										*/

#define DECLARE_QUEUES(proto)      \
	struct s_q_##proto {           \
		struct pico_queue in, out; \
	} q_##proto

#ifdef PICO_SUPPORT_TICKLESS
#define ATTACH_QUEUES_LISTENERS(St, pname, P)                                             \
	do {                                                                                  \
		pico_queue_register_listener(St, &((St->q_ ## pname.in, proto_full_loop_in, P); \
        pico_queue_register_listener(St, &((St->q_ ## pname.out, proto_full_loop_out, P); \
	} while (0)
#else
#define ATTACH_QUEUES_LISTENERS(St, pname, P) \
	do {                                      \
	} while (0)
#endif

#define ATTACH_QUEUES(St, pname, P)            \
	do {                                       \
		P.q_in = &((St)->q_##pname.in);        \
		P.q_out = &((St)->q_##pname.out);      \
		ATTACH_QUEUES_LISTENERS(St, pname, P); \
	} while (0)

#define EMPTY_TREE(name, comp) \
	do {                       \
		name.root = &LEAF;     \
		name.compare = comp;   \
	} while (0)

struct pico_timer {
	void *arg;
	void (*timer)(pico_time timestamp, void *arg);
};

struct pico_timer_ref {
	pico_time expire;
	uint32_t id;
	uint32_t hash;
	struct pico_timer *tmr;
};

typedef struct pico_timer_ref pico_timer_ref;

DECLARE_HEAP(pico_timer_ref, expire)

struct pico_stack {
	struct pico_scheduler *sched;
	heap_pico_timer_ref *Timers;
	uint32_t timer_id;
	int score[PROTO_DEF_NR];
	int index[PROTO_DEF_NR];
	int avg[PROTO_DEF_NR][PROTO_DEF_AVG_NR];
	int ret[PROTO_DEF_NR];
	struct s_devices_rr_info {
		struct pico_tree_node *node_in, *node_out;
	} Devices_rr_info;
	struct pico_tree Device_tree;
	struct pico_tree Hotplug_device_tree;
	uint32_t hotplug_timer_id;
	volatile pico_time pico_tick;

#ifdef PICO_SUPPORT_ETH
	DECLARE_QUEUES(ethernet);
#endif

#ifdef PICO_SUPPORT_PACKET_SOCKETS
	DECLARE_QUEUES(proto_ll);
	uint16_t PSocket_id;
	struct pico_tree PSockets;
#endif

#ifdef PICO_SUPPORT_6LOWPAN
	DECLARE_QUEUES(sixlowpan);
	DECLARE_QUEUES(sixlowpan_ll);
	struct pico_tree SixLowPanCTXTree;
	struct pico_tree LPFragTree;
	struct pico_tree LPReassemblyTree;
	uint16_t lowpan_dgram_tag;
#endif

#ifdef PICO_SUPPORT_IPV4
	DECLARE_QUEUES(ipv4);
	struct pico_tree Tree_dev_link;
	struct pico_tree Routes;
	uint16_t ipv4_progressive_id;
	struct pico_ipv4_route *default_bcast_route;
#ifdef PICO_SUPPORT_RAWSOCKETS
	uint16_t IP4Socket_id;
	struct pico_tree IP4Sockets;
#endif
#ifdef PICO_SUPPORT_NAT
	struct pico_tree NATOutbound;
	struct pico_tree NATInbound;
#endif
#ifdef PICO_SUPPORT_IPV4FRAG
	uint32_t ipv4_cur_frag_id;
	uint32_t ipv4_fragments_timer;
	struct pico_tree ipv4_fragments;
#endif
	uint16_t ipv4_pre_forward_last_id;
	uint16_t ipv4_pre_forward_last_proto;
	struct pico_ip4 ipv4_pre_forward_last_src;
	struct pico_ip4 ipv4_pre_forward_last_dst;
#ifdef PICO_SUPPORT_MCAST
	/* Default network interface for multicast transmission */
	struct pico_ipv4_link *ipv4_mcast_default_link;
#endif
#endif

#ifdef PICO_SUPPORT_IPV6
	DECLARE_QUEUES(ipv6);
	struct pico_tree IPV6Routes, IPV6Links, Tree_dev_ip6_link;
	struct pico_tree IPV6NQueue;
	struct pico_tree IPV6NCache;
	struct pico_tree IPV6RCache;
#ifdef PICO_SUPPORT_IPV6FRAG
	uint32_t ipv6_cur_frag_id;
	uint32_t ipv6_fragments_timer;
	struct pico_tree ipv6_fragments;
#endif
#ifdef PICO_SUPPORT_IPV6PMTU
	struct pico_tree IPV6PathCache;
	struct pico_ipv6_path_timer ipv6_path_cache_gc_timer;
#endif
#ifdef PICO_SUPPORT_MCAST
	/* Default network interface for multicast transmission */
	struct pico_ipv6_link *ipv6_mcast_default_link;
#endif
#endif

#if defined(PICO_SUPPORT_MLD) && defined(PICO_SUPPORT_IPV6) && \
	defined(PICO_SUPPORT_MCAST)
	struct pico_tree MLDTimers;
	struct pico_tree MLDParameters;
	struct pico_tree MLDAllow;
	struct pico_tree MLDBlock;
#endif

#ifdef PICO_SUPPORT_ICMP4
	DECLARE_QUEUES(icmp4);
	struct pico_tree Pings;
	struct pico_tree ICMP4Sockets;
#endif

#ifdef PICO_SUPPORT_ICMP6
	DECLARE_QUEUES(icmp6);
	struct pico_tree IPV6Pings;
#endif

#if defined(PICO_SUPPORT_IGMP) && defined(PICO_SUPPORT_MCAST)
	DECLARE_QUEUES(igmp);
	struct pico_tree IGMPParameters, IGMPTimers, IGMPAllow, IGMPBlock;
	struct pico_tree MCASTSockets, MCASTFilter, MCASTFilter_ipv6;
#endif

#ifdef PICO_SUPPORT_UDP
	DECLARE_QUEUES(udp);
	struct pico_tree UDPTable;
#endif

#ifdef PICO_SUPPORT_TCP
	DECLARE_QUEUES(tcp);
	struct pico_tree TCPTable;
#endif

#if defined(PICO_SUPPORT_TCP) || defined(PICO_SUPPORT_UDP)
	struct pico_sockport *sp_udp, *sp_tcp;
	struct pico_tree_node *tcp_loop_index, *udp_loop_index;
#ifdef PICO_SUPPORT_MUTEX
	void *SockMutex;
#endif
#endif

#ifdef PICO_SUPPORT_DHCPC
	struct pico_tree DHCPCookies;
	char dhcpc_host_name[PICO_DHCP_HOSTNAME_MAXLEN];
	char dhcpc_domain_name[PICO_DHCP_HOSTNAME_MAXLEN];
#endif

#ifdef PICO_SUPPORT_DHCPD
	struct pico_tree DHCPSettings;
	struct pico_tree DHCPNegotiations;
#endif

#if ((defined PICO_SUPPORT_IPV4) && (defined PICO_SUPPORT_ETH))
	struct pico_tree arp_tree;
	/* Callback handler for ip conflict service (e.g. IPv4 SLAAC)
	 *  Whenever the IP address registered here is seen in the network,
	 *  the callback is awaken to take countermeasures against IP collisions.
	 *
	 */
	struct arp_service_ipconflict conflict_ipv4;
#endif

#ifdef PICO_SUPPORT_AODV
	struct pico_socket *aodv_socket;
	struct pico_tree aodv_nodes;
	struct pico_tree aodv_devices;
	uint32_t pico_aodv_local_id;
#endif

#ifdef PICO_SUPPORT_DNS_CLIENT
	struct pico_tree NSTable;
	struct pico_tree DNSTable;
#endif
#ifdef PICO_SUPPORT_MDNS
#if (PICO_MDNS_ALLOW_CACHING == 1)
	/* S->MDNSCache records from mDNS peers on the network */
	struct pico_tree MDNSCache;
#endif
	/* My records for which I want to have the authority */
	struct pico_tree MDNSOwnRecords;
	/* Cookie-tree */
	struct pico_tree MDNSCookies;
	/* MDNS Communication variables */
	struct pico_socket *mdns_sock_ipv4;
	uint16_t mdns_port;
	/* ****************************************************************************
	 *  Hostname for this machine, only 1 hostname can be set.
	 *  Following RFC6267: 15.4 Recommendation
	 * *****************************************************************************/
	char *mdns_hostname;
	void (*mdns_init_callback)(pico_mdns_rtree *, char *, void *);

/* ****************************************************************************
 *  Hostname for this machine, only 1 hostname can be set.
 *  Following RFC6267: 15.4 Recommendation
 * *****************************************************************************/
#endif

#ifdef PICO_SUPPORT_IPFILTER
	struct pico_tree ipfilter_tree;
#endif

#ifdef PICO_SUPPORT_TICKLESS
	struct pico_job *pico_jobs_backlog;
	struct pico_job *pico_jobs_backlog_tail;
#endif

#ifdef PICO_SUPPORT_TFTP
	struct pico_tftp_server_t tftp_server;
	struct pico_tftp_session *tftp_sessions;
#endif

#ifdef PICO_SUPPORT_OLSR
	struct pico_socket *pico_olsr_socket;
	uint16_t pico_olsr_ansn;
	struct olsr_route_entry *OLSRLocal_interfaces;
	struct olsr_dev_entry *OLSRLocal_devices;
	uint16_t pico_olsr_msg_counter;
#endif

#ifdef PICO_SUPPORT_SNTP_CLIENT
	uint16_t sntp_port;
	struct pico_timeval sntp_server_time;
	pico_time sntp_tick_stamp;
#endif
};

#define PICO_MAX_TIMERS 20

#define PICO_ETH_MRU (1514u)
#define PICO_IP_MRU (1500u)

/*******************************************************************************
 *  TRANSPORT LAYER
 ******************************************************************************/

/* From dev up to socket */
int32_t pico_transport_receive(struct pico_frame *f, uint8_t proto);

/*******************************************************************************
 *  NETWORK LAYER
 ******************************************************************************/

/* From socket down to dev */
int32_t pico_network_send(struct pico_frame *f);

/* From dev up to socket */
int32_t pico_network_receive(struct pico_frame *f);

/*******************************************************************************
 *  DATALINK LAYER
 ******************************************************************************/

/* From socket down to dev */
int pico_datalink_send(struct pico_frame *f);

/* From dev up to socket */
int pico_datalink_receive(struct pico_frame *f);

/*******************************************************************************
 *  PHYSICAL LAYER
 ******************************************************************************/

/* Enqueues the frame in the device-queue. From socket down to dev */
int32_t pico_sendto_dev(struct pico_frame *f);

/* LOWEST LEVEL: interface towards stack from devices */
/* Device driver will call this function which returns immediately.
 * Incoming packet will be processed later on in the dev loop.
 * The zerocopy version will associate the current buffer to the newly created
 * frame. Warning: the buffer used in the zerocopy version MUST have been
 * allocated using PICO_ZALLOC()
 */
int32_t pico_stack_recv(struct pico_device *dev, uint8_t *buffer, uint32_t len);
int32_t pico_stack_recv_zerocopy(struct pico_device *dev, uint8_t *buffer,
								 uint32_t len);
int32_t pico_stack_recv_zerocopy_ext_buffer(struct pico_device *dev,
											uint8_t *buffer, uint32_t len);
int32_t pico_stack_recv_zerocopy_ext_buffer_notify(
	struct pico_device *dev, uint8_t *buffer, uint32_t len,
	void (*notify_free)(uint8_t *buffer));
struct pico_frame *pico_stack_recv_new_frame(struct pico_device *dev,
											 uint8_t *buffer, uint32_t len);

/* ===== SENDING FUNCTIONS (from socket down to dev) ===== */

int32_t pico_network_send(struct pico_frame *f);
int32_t pico_sendto_dev(struct pico_frame *f);

#ifdef PICO_SUPPORT_ETH
int32_t pico_ethernet_send(struct pico_stack *S, struct pico_frame *f);

/* The pico_ethernet_receive() function is used by
 * those devices supporting ETH in order to push packets up
 * into the stack.
 */
/* DATALINK LEVEL */
int32_t pico_ethernet_receive(struct pico_stack *S, struct pico_frame *f);
#else
/* When ETH is not supported by the stack... */
#define pico_ethernet_send(f) (-1)
#define pico_ethernet_receive(f) (-1)
#endif

/* ----- Initialization & tick ----- */
int pico_stack_init(struct pico_stack **S);
void pico_stack_tick(struct pico_stack *S);

struct pico_stack *pico_get_default_stack(void);

/* ---- Notifications for stack errors */
int pico_notify_socket_unreachable(struct pico_stack *S, struct pico_frame *f);
int pico_notify_proto_unreachable(struct pico_stack *S, struct pico_frame *f);
int pico_notify_dest_unreachable(struct pico_stack *S, struct pico_frame *f);
int pico_notify_ttl_expired(struct pico_stack *S, struct pico_frame *f);
int pico_notify_frag_expired(struct pico_stack *S, struct pico_frame *f);
int pico_notify_pkt_too_big(struct pico_stack *S, struct pico_frame *f);

/* Various. */
int pico_source_is_local(struct pico_stack *S, struct pico_frame *f);
int pico_frame_dst_is_unicast(struct pico_stack *S, struct pico_frame *f);
void pico_store_network_origin(void *src, struct pico_frame *f);
uint32_t pico_timer_add(struct pico_stack *S, pico_time expire,
						void (*timer)(pico_time, void *), void *arg);
uint32_t pico_timer_add_hashed(struct pico_stack *S, pico_time expire,
							   void (*timer)(pico_time, void *), void *arg,
							   uint32_t hash);
void pico_timer_cancel_hashed(struct pico_stack *S, uint32_t hash);
void pico_timer_cancel(struct pico_stack *S, uint32_t id);
extern uint32_t pico_rand(void);
void pico_to_lowercase(char *str);
int pico_address_compare(union pico_address *a, union pico_address *b,
						 uint16_t proto);
int32_t pico_seq_compare(uint32_t a, uint32_t b);

#ifdef PICO_SUPPORT_TICKLESS
long long int pico_stack_go(struct pico_stack *S);
#endif

void pico_stack_deinit(struct pico_stack *S);

#endif
