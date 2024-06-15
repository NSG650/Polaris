#include <cstdio>

int main(int argc, char **argv) {
	printf("Hello, ");
	if (argc == 1) {
		printf("There\n");
		return 0;
	}
	argv++;
	while (*argv) {
		printf("%s ", *argv++);
	}
	printf("\n");
	return 0;
}
