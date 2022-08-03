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

int open(char *path, int32_t mode) {
	return syscall2(0x2, (uint64_t)path, mode);
}

int64_t read(int fd, void *data, size_t count) {
	return syscall3(0x0, fd, (uint64_t)data, count);
}

int64_t write(int fd, void *data, size_t count) {
	return syscall3(0x1, fd, (uint64_t)data, count);
}

void main(void) {
	puts("Hello I am supposed to be the init\n");
	int fd = open("/fun/hi.txt", 0x0);
	char fun[128] = {0};
	puts("Reading /fun/hi.txt: \n");
	read(fd, fun, 128);
	puts(fun);

	fd = open("/file_from_user.txt", 0x1);
	char data[] = "This file was created from user\n";
	write(fd, data, sizeof(data));
	read(fd, fun, 128);
	puts("/file_from_user.txt\n");
	puts(fun);
}
