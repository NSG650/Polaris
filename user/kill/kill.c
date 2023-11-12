#include <stddef.h>
#include <stdint.h>

extern uint64_t syscall0(uint64_t syscall_number);
extern uint64_t syscall1(uint64_t syscall_number, uint64_t args0);
extern uint64_t syscall2(uint64_t syscall_number, uint64_t args0,
						 uint64_t args1);
extern uint64_t syscall3(uint64_t syscall_number, uint64_t args0,
						 uint64_t args1, uint64_t args2);

void puts(char *string) {
	syscall1(0x67, (uint64_t)string);
}

size_t write(int fd, const void *data, size_t count) {
	return syscall3(0x1, fd, (uint64_t)data, count);
}

size_t read(int fd, void *data, size_t count) {
	return syscall3(0x0, fd, (uint64_t)data, count);
}

int simple_atoi(char *str) {
	int res = 0;

	for (int i = 0; str[i] != '\0'; ++i)
		res = res * 10 + str[i] - '0';

	return res;
}

void main(void) {
	char message[] =
		"Please enter the pid of the process you would like to kill\n";
	write(1, message, sizeof(message));

	char pid_to_read[256] = {0};
	read(0, pid_to_read, 256);

	int pid = simple_atoi(pid_to_read);
	if (pid < 2) {
		char nope[] = "I can't kill the kernel or init!\n";
		write(1, nope, sizeof(nope));
		syscall1(0x3c, 1);
	}

	int ret = syscall2(0x3e, pid, 9);
	if (ret == -1) {
		char nope[] = "Seems like a process with that pid does not exist!\n";
		write(1, nope, sizeof(nope));
		syscall1(0x3c, 1);
	}

	syscall1(0x3c, 0);
}
