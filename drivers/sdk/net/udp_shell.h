#ifndef UDP_SHELL_H
#define UDP_SHELL_H

#include <klibc/hashmap.h>
#include <stddef.h>
#include <stdint.h>

struct udp_shell_data {
	char *buffer;
	uint16_t source_port;
	uint8_t ip[4];
	uint8_t mac[4];
};

void udp_shell_entry_add(uint8_t *mac, struct udp_shell_data *data);
struct udp_shell_data *udp_shell_entry_get(uint8_t *mac);
void udp_shell_init(void);

#endif
