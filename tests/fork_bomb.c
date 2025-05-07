#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
	for (;;) {
		fork();
	}
	return 0;
}
