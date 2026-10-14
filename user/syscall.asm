section .text
global syscall0
global syscall1
global syscall2
global syscall3
global syscall4
global syscall5
global syscall6
global die
global px_fork

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

syscall4:
	mov rax, rdi
	mov rdi, rsi
	mov rsi, rdx
	mov rdx, rcx
	mov r10, r8
	syscall
	ret

syscall5:
	mov rax, rdi
	mov rdi, rsi
	mov rsi, rdx
	mov rdx, rcx
	mov r10, r8
	mov r8, r9
	syscall
	ret

; Disgusting hack, drops r12 current value
syscall6:
	mov rax, rdi
	mov rdi, rsi
	mov rsi, rdx
	mov rdx, rcx
	mov r10, r8
	mov r8, r9
	pop r12
	pop r9
	syscall
	push r12
	ret

die:
    ud2

extern _px_fork

px_fork:
    mov  rdi, [rsp]
    lea  rsi, [rsp + 8]
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov  rdx, rsp
    sub  rsp, 8
    call _px_fork
    add  rsp, 8
    pop  r15
    pop  r14
    pop  r13
    pop  r12
    pop  rbx
    pop  rbp
    ret
