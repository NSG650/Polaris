#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
	printf("Hi\n");
	fork();
	return execvp(argv[0], argv);
}
