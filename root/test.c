int printf(const char *format, ...);
void process_exit(void);
void thread_exit(unsigned long int return_val);

char some_array[16] = {0};
char other_array[16] = {0};

int some_var = 123;

static void fast_fib(unsigned long long int n, unsigned long long int m[]) {
	unsigned long long int a, b, c, d;
	if (n == 0) {
		m[0] = 0;
		m[1] = 1;
		return;
	}
	unsigned long long int s[2] = {0};
	fast_fib(n / 2, s);
	a = s[0]; // F(N)
	b = s[1]; // F(N + 1)
	c = a * (b * 2 - a);
	d = a * a + b * b;
	if (!(n & 1)) {
		m[0] = c;
		m[1] = d;
		return;
	} else {
		m[0] = d;
		m[1] = c + d;
		return;
	}
}

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
	unsigned long long int s[2] = {0};
	fast_fib(10, s);
	printf("fast_fib(10, s): n: %llu, n + 1: %llu", s[0], s[1]);
	thread_exit(0);
}
