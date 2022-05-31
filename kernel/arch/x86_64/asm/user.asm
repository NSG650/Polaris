section .text
global user_thread

die:
	jmp die

user_thread:
	xor rax, rax
	mov rdi, msg
	int 0x80
	jmp die

section .data
	msg: db "Hello from user!", 0xa, 0x0
