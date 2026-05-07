#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>

int main(int argc, char **argv) {
	if (argc < 2) {
		printf("Usage: %s domain.name\n", argv[0]);
		return -1;
	}
	struct hostent *h = gethostbyname(argv[1]);
	if (h == NULL) {
		perror("gethostbyname");
		return -11;
	}
	printf("resolved: %s\n", inet_ntoa(*(struct in_addr *)h->h_addr));
	return 0;
}
