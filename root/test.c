int printf(const char *format, ...);
void process_exit(void);
void thread_exit(unsigned long int return_val);

char some_array[16] = {0};
char other_array[16] = {0};

int some_var = 123;

static long long fib(long long i) {
	if (i <= 1)
		return i;
	return fib(i - 1) + fib(i - 2);
}

static void other_func() {
	printf("Hello world!\n");
	printf("some_array[0]: %d\n", some_array[0]);
	other_array[0] = 12;
	some_var = 0xDEADBEEF;
}

void run() {
	some_array[0] = 1;
	printf("I am an ELF binary!\n");
	other_func();
	printf("other_array[0]: %d\n", other_array[0]);
	printf("some_var: %X\n", some_var);
	printf("fib(10): %lld\n", fib(10));
	thread_exit(0);
}
