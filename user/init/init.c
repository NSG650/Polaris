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
	syscall2(0x2, (uint64_t)path, mode);
}

void main(void) {
	puts("Hello I am supposed to be the init\n");
	open("/fun/hi.txt", 0x2);
}
