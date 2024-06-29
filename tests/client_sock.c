#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

int main(void) {
	printf("[*] Client: Hello World!\n");

	int server_sock = -1;
	struct sockaddr_un server_addr = {0};
	int connection_result = -1;

	char ch = 'Q';

	server_sock = socket(AF_UNIX, SOCK_STREAM, 0);

	server_addr.sun_family = AF_UNIX;
	strcpy(server_addr.sun_path, "funny_socket");

	connection_result = connect(server_sock, (struct sockaddr *)&server_addr,
								sizeof(server_addr));

	if (connection_result < 0) {
		printf("[!] Client: Failed to connect to socket\n");
		return -1;
	}

	write(server_sock, &ch, 1);
	read(server_sock, &ch, 1);

	if (ch == 'Q') {
		printf("[+] Client: Got expected result (%c)\n", ch);
	} else {
		printf("[!] Client: Got unexpected result (%c)\n", ch);
	}

	close(server_sock);
	return 0;
}
