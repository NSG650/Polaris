#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
	int ret = fork();
	if (ret == 0) {
		printf("I am the child process!\n");
		for (int i = 0; i < 1000; i++) {
			printf("Hello! %d\n", i);
		}
		exit(0);
	}
	int status = 0;
	while (waitpid(ret, &status, WNOHANG) == 0) {
		;
	}
	printf("Done!\n");
	return 0;
}
