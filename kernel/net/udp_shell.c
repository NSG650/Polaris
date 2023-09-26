#include <net/udp.h>
#include <net/udp_shell.h>

static HASHMAP_TYPE(struct udp_shell_data *) udp_shells;

void udp_shell_entry_add(uint8_t *mac, struct udp_shell_data *data) {
	(void)mac;
	(void)data;
	// HASHMAP_INSERT(&udp_shells, mac, 6, data);
}

struct udp_shell_data *udp_shell_entry_get(uint8_t *mac) {
	struct udp_shell_data *entry = NULL;
	HASHMAP_GET(&udp_shells, entry, mac, 6);
	return entry;
}

void udp_shell_init(void) {
	(void)udp_shells;
	return;
	// udp_shells = (typeof(udp_shells))HASHMAP_INIT(256);
}
