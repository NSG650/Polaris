section .text
global _start

die:
	jmp die

_start:
	xor rax, rax
	mov rdi, msg
	int 0x80
	jmp die

section .data
	msg: db "Hello I am supposed to be the init", 0xa, 0x0
