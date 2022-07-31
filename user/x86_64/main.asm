section .text
global _start
global syscall0
global syscall1
global syscall2
global syscall3
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

syscall2:
	mov rax, rdi
	mov rdi, rsi
	mov rsi, rdx
	syscall
	ret

syscall3:
	mov rax, rdi
	mov rdi, rsi
	mov rsi, rdx
	mov rdx, rcx
	syscall
	ret

die:
	jmp die

_start:
	call main
	jmp die
