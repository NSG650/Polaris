#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
	for (int i = 0; i < 100; i++) {
		int ret = fork();
		if (ret == 0) {
			printf("I am pid %d\n", getpid() + 10000);
			exit(0);
		}
	}
	return 0;
}
