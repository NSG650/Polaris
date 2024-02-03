global _start

section .text

_start:
	mov rax, 1
	mov rdi, 1
	mov rsi, msg
	mov rdx, msg_length
	syscall

	mov rax, 60
	xor rdi, rdi
	syscall

section .rodata
	msg: db "Hello World!", 0xa
	msg_length: equ $ - msg
