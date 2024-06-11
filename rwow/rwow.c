#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv, char **envp) {
	if (argc < 2) {
		printf("Usage: %s <command to run>\n", argv[0]);
		return -1;
	}

	if (fork() == 0) {
		if (execve(argv[1], &argv[1], envp) < 0) {
			printf("[!] Failed to execve!\n");
			exit(-1);
		}
	}

	return 0;
}
