#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
	printf("Hi\n");
	return execvp(argv[0], argv);
}
