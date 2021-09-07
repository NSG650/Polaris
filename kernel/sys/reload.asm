extern gdt_pointer
global gdt_reload

gdt_reload:
	lgdt [gdt_pointer]
	push 8
	push .flush
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
