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


void main(void) {
	char message[] = "Hello I am test program\n";
	write(1, message, sizeof(message));
	syscall1(0x3c, 0);
}
