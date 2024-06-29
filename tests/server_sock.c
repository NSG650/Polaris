#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

int main(void) {
	printf("[*] Server: Hello World!\n");

	int server_sock = -1;
	int client_sock = -1;

	struct sockaddr_un server_addr = {0};
	struct sockaddr_un client_addr = {0};

	server_sock = socket(AF_UNIX, SOCK_STREAM, 0);

	server_addr.sun_family = AF_UNIX;
	strcpy(server_addr.sun_path, "funny_socket");

	int slen = sizeof(server_addr);

	if (bind(server_sock, (struct sockaddr *)&server_addr, slen) < 0) {
		printf("[!] Server: Failed to bind\n");
		return -1;
	}

	listen(server_sock, 5);

	for (;;) {
		char ch = 0;
		socklen_t client_length = sizeof(client_addr);
		client_sock = accept(server_sock, (struct sockaddr *)&client_addr,
							 &client_length);
		read(client_sock, &ch, 1);
		if (ch == 'Q') {
			printf("[+] Server: Got expected result (%c) exiting now\n", ch);
			write(client_sock, &ch, 1);
			close(client_sock);
			exit(0);
		} else {
			printf("[!] Server: Got unexpected result (%c)\n", ch);
			write(client_sock, &ch, 1);
			close(client_sock);
		}
	}

	return 0;
}
