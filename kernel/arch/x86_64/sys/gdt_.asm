extern gdt_pointer
global gdt_reload

gdt_reload:
	lgdt [rel gdt_pointer]
	push 8
	lea rax, [rel .flush]
	push rax
	retfq
.flush:
	mov eax, 0x10
	mov ds, eax
	mov es, eax
	mov fs, eax
	mov gs, eax
	mov ss, eax
	ret

global tss_reload
tss_reload:
	mov ax, 0x2B
	ltr ax
	ret
