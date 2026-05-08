#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <unistd.h>

#define PORT 80
#define BUFFER_SIZE 4096

struct client_args {
	int fd;
	struct sockaddr_in addr;
};

char HTML[512] = {0};
char HTTP_RESPONSE[1024] = {0};

static void *handle_client(void *arg) {
	struct client_args *client = (struct client_args *)arg;
	char buf[BUFFER_SIZE];

	printf("Connection from %s\n", inet_ntoa(client->addr.sin_addr));

	ssize_t n = read(client->fd, buf, sizeof(buf) - 1);
	if (n > 0) {
		buf[n] = '\0';
		printf("--- Request ---\n%s\n", buf);
		write(client->fd, HTTP_RESPONSE, strlen(HTTP_RESPONSE));
	}

	close(client->fd);
	free(client);
	return NULL;
}

int main(void) {
	int server_fd, client_fd;
	struct sockaddr_in addr;
	char buf[BUFFER_SIZE];
	int opt = 1;
	struct utsname system_uname = {0};
	uname(&system_uname);

	snprintf(HTML, 512,
			 "<h1>It works!</h1><p>Hello from %s %s %s!</p><a "
			 "href=\"https://github.com/NSG650/%s\"> What is %s?</a>\r\n",
			 system_uname.sysname, system_uname.release, system_uname.version,
			 system_uname.sysname, system_uname.sysname);

	snprintf(HTTP_RESPONSE, sizeof(HTTP_RESPONSE),
			 "HTTP/1.1 200 OK\r\n"
			 "Content-Type: text/html\r\n"
			 "Connection: close\r\n"
			 "Server: %s %s\r\n"
			 "\r\n"
			 "%s",
			 system_uname.sysname, system_uname.version, HTML);

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

		struct client_args *args = malloc(sizeof(struct client_args));
		if (args == NULL) {
			close(client_fd);
			continue;
		}
		args->fd = client_fd;
		args->addr = client_addr;

		pthread_t thread;
		if (pthread_create(&thread, NULL, handle_client, args) != 0) {
			perror("pthread_create");
			close(client_fd);
			free(args);
			continue;
		}
		pthread_detach(thread);
	}

	close(server_fd);
	return 0;
}
