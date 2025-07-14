#include <pthread.h>
#include <stdio.h>

void *thread_function(void *arg) {
	(void)arg;
	printf("Hello from thread\n");
	return NULL;
}

int main(void) {
	pthread_t thread = {0};
	pthread_create(&thread, NULL, thread_function, NULL);
	pthread_join(thread, NULL);
	printf("Done!\n");
	return 0;
}
