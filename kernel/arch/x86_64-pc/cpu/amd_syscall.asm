section .text

extern syscall_handler
global amd_syscall_entry
amd_syscall_entry:
	swapgs

	mov qword [gs:0016], rsp
	mov rsp, qword [gs:0008]

	push 0x1b            ; ss
	push qword [gs:0016] ; rsp
	push r11             ; rflags
	push 0x23            ; cs
	push rcx             ; rip

	sub rsp, 24

	push rax
	push rbx
	push rcx
	push rdx
	push rbp
	push rdi
	push rsi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	mov rdi, rsp
	sti
	call syscall_handler

	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rsi
	pop rdi
	pop rbp
	pop rdx
	pop rcx
	pop rbx
	pop rax

	add rsp, 24

	cli

	mov rsp, qword [gs:0016]

	swapgs

	o64 sysret
