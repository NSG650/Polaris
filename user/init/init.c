#include <stdint.h>
#include <stddef.h>

extern uint64_t syscall0(uint64_t args0);
extern uint64_t syscall1(uint64_t args0, uint64_t args1);

void puts(char *string) {
    syscall1(0, (uint64_t)string);
}

void main(void) {
    puts("Hello I am supposed to be the init\n");
    if (syscall0(1) == 0xff)
        puts("The computer is on!\n");
}