#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
	for (int i = 0; i < 10000; i++) {
		int ret = fork();
		if (ret == 0) {
			printf("I am pid %d\n", getpid());
			char *args[] = {"uname", "-a", NULL};
			execvp(args[0], args);
			exit(0);
		}
	}
	return 0;
}
