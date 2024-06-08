global resched_context_switch

resched_context_switch:
	mov rsp, rdi
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
	swapgs
	iretq
