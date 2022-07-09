section .text
global _start
global syscall0
global syscall1
extern main

syscall0:
	mov rax, rdi
	syscall
	ret

syscall1:
	mov rax, rdi
	mov rdi, rsi
	syscall
	ret

die:
	jmp die

_start:
	call main
	jmp die
