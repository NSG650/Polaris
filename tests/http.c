#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <unistd.h>

#define PORT 80
#define BUFFER_SIZE 4096

int main(void) {
	int server_fd, client_fd;
	struct sockaddr_in addr;
	char buf[BUFFER_SIZE];
	int opt = 1;
	struct utsname system_uname = {0};
	uname(&system_uname);

	char HTTP_RESPONSE[1024] = "HTTP/1.1 200 OK\r\nContent-Type: "
							   "text/html\r\nConnection: close\r\n\r\n";
	char HTML[512] = {0};
	snprintf(HTML, 512, "<h1>It works!</h1><p>Hello from %s %s %s!</p>\r\n",
			 system_uname.sysname, system_uname.release, system_uname.version);
	strcat(HTTP_RESPONSE, HTML);

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		perror("socket");
		return -1;
	}

	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT);

	if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return -1;
	}

	if (listen(server_fd, 10) < 0) {
		perror("listen");
		return -1;
	}

	printf("Listening on port %d\n", PORT);

	while (1) {
		struct sockaddr_in client_addr;
		socklen_t client_len = sizeof(client_addr);

		client_fd =
			accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
		if (client_fd < 0) {
			perror("accept");
			continue;
		}

		printf("Connection from %s\n", inet_ntoa(client_addr.sin_addr));

		ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
		if (n > 0) {
			buf[n] = '\0';
			printf("--- Request ---\n%s\n", buf);
			write(client_fd, HTTP_RESPONSE, strlen(HTTP_RESPONSE));
		}

		close(client_fd);
	}

	close(server_fd);
	return 0;
}
