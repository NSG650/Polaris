#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
	for (int i = 0; i < 100; i++) {
		int ret = fork();
		if (ret == 0) {
			if (i & 1) {
				exit(1);
			}
			char *args[] = {"uname", "-a", NULL};
			execvp(args[0], args);
			exit(0);
		}
		int status = 0;
		waitpid(ret, &status, 0);
		printf("Status: %d\n", status);
	}
}
