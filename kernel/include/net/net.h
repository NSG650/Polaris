#ifndef NET_H
#define NET_H

#include <stddef.h>
#include <stdint.h>

#define BSWAP16(x) ((((x) & 0xff) << 8) | (((x) & 0xff00) >> 8))

#define BSWAP32(x)                                            \
	((((x) & 0x000000ff) << 24) | (((x) & 0x0000ff00) << 8) | \
	 (((x) & 0x00ff0000) >> 8) | (((x) & 0xff000000) >> 24));

#define REQ_TYPE_ARP 0x0806
#define REQ_TYPE_IP 0x0800

#define INTERFACE_ETH 0
#define INTERFACE_LOOPBACK 1

#define IFF_UP (1 << 0)
#define IFF_LOOPBACK (1 << 3)
#define IFF_RUNNING (1 << 6)

#define SIOCGIFNAME 0x8910	  /* get iface name		*/
#define SIOCSIFLINK 0x8911	  /* set iface channel		*/
#define SIOCGIFCONF 0x8912	  /* get iface list		*/
#define SIOCGIFFLAGS 0x8913	  /* get flags			*/
#define SIOCSIFFLAGS 0x8914	  /* set flags			*/
#define SIOCGIFADDR 0x8915	  /* get PA address		*/
#define SIOCSIFADDR 0x8916	  /* set PA address		*/
#define SIOCGIFDSTADDR 0x8917 /* get remote PA address	*/
#define SIOCSIFDSTADDR 0x8918 /* set remote PA address	*/
#define SIOCGIFBRDADDR 0x8919 /* get broadcast PA address	*/
#define SIOCSIFBRDADDR 0x891a /* set broadcast PA address	*/
#define SIOCGIFNETMASK 0x891b /* get network PA mask		*/
#define SIOCSIFNETMASK 0x891c /* set network PA mask		*/
#define SIOCGIFGATEWAY 0x891d /* get gateway			*/
#define SIOCSIFGATEWAY 0x891e /* set gateway			*/
#define SIOCGIFMEM 0x891f	  /* get memory address (BSD)	*/
#define SIOCSIFMEM 0x8920	  /* set memory address (BSD)	*/
#define SIOCGIFMTU 0x8921	  /* get MTU size			*/
#define SIOCSIFMTU 0x8922	  /* set MTU size			*/
#define SIOCSIFNAME 0x8923	  /* set interface name */
#define SIOCSIFHWADDR 0x8924  /* set hardware address 	*/
#define SIOCGIFENCAP 0x8925	  /* get/set encapsulations       */
#define SIOCSIFENCAP 0x8926
#define SIOCGIFHWADDR 0x8927 /* Get hardware address		*/
#define SIOCGIFSLAVE 0x8929	 /* Driver slaving support	*/
#define SIOCSIFSLAVE 0x8930
#define SIOCADDMULTI 0x8931 /* Multicast address lists	*/
#define SIOCDELMULTI 0x8932
#define SIOCGIFINDEX 0x8933		 /* name -> if_index mapping	*/
#define SIOGIFINDEX SIOCGIFINDEX /* misprint compatibility :-)	*/
#define SIOCSIFPFLAGS 0x8934	 /* set/get extended flags set	*/
#define SIOCGIFPFLAGS 0x8935
#define SIOCDIFADDR 0x8936		  /* delete PA address		*/
#define SIOCSIFHWBROADCAST 0x8937 /* set hardware broadcast addr	*/
#define SIOCGIFCOUNT 0x8938		  /* get number of devices */

struct network_packet {
	uint8_t destination_mac[6];
	uint8_t source_mac[6];
	uint16_t type;
	uint8_t data[];
} __attribute__((packed));

struct net_nic_interfaces {
	char name[16];
	uint32_t flags;
	uint32_t type;
	uint32_t mtu;
	uint8_t ip_address[4];
	uint8_t subnet[4];
	uint8_t gateway[4];

	uint8_t *(*get_mac_addr)(void);
	void (*send_packet)(uint8_t *dest, void *packet, uint32_t packet_length,
						uint16_t protocol);
};

void net_handle_packet_thread(uint64_t *handover);
void net_handle_packet(void *packet, uint16_t packet_length,
					   struct net_nic_interfaces *nic_interfaces);
void net_init(void);

#endif
