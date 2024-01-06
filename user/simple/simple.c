#include <stddef.h>
#include <stdint.h>

struct timespec {
	int64_t tv_sec;
	long tv_nsec;
};

extern uint64_t syscall0(uint64_t syscall_number);
extern uint64_t syscall1(uint64_t syscall_number, uint64_t args0);
extern uint64_t syscall2(uint64_t syscall_number, uint64_t args0,
						 uint64_t args1);
extern uint64_t syscall3(uint64_t syscall_number, uint64_t args0,
						 uint64_t args1, uint64_t args2);

void puts(char *string) {
	syscall1(0x67, (uint64_t)string);
}

int getpid(void) {
	return (int)syscall0(0x27);
}

int fork(void) {
	return (int)syscall0(0x39);
}

int execve(char *path, char **argv, char **envp) {
	return syscall3(0x3b, (uint64_t)path, (uint64_t)argv, (uint64_t)envp);
}

_Noreturn void exit(uint8_t exit_code) {
	syscall1(0x3c, exit_code);
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
	return syscall2(0x23, (uint64_t)req, (uint64_t)rem);
}

int msleep(long msec) {
	struct timespec ts = {0};
	int res = 0;

	if (msec <= 0) {
		return -1;
	}

	ts.tv_sec = msec / 1000;
	ts.tv_nsec = (msec % 1000) * 1000000;

	nanosleep(&ts, &ts);

	return res;
}

void main(void) {
	puts("Hello world!\n");
	if (getpid() != 1) {
		exit(0);
	}
	if (fork() == 0) {
		puts("I am the forked child process!\n");
		if (fork() == 0) {
			char *argv[] = {"/bin/hello", NULL};
			char *envp[] = {"USER=ROOT", "HOME=/root",
							"PATH=/bin:/usr/bin:/usr/local/bin", "TERM=linux",
							NULL};
			if (execve(argv[0], argv, envp) == -1) {
				puts("Failed to execve :(\n");
			}
		}
		exit(0);
	}
	puts("I am the parent process\n");
}
