section .text
global _start
global syscall0
global syscall1
extern main

syscall0:
	mov rax, rdi
	int 0x80
	ret

syscall1:
	mov rax, rdi
	mov rdi, rsi
	int 0x80
	ret

die:
	jmp die

_start:
	call main
	jmp die
