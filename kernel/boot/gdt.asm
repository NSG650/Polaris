[bits 64]

; Copyright 2021 NSG650
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;     http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS,
; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; See the License for the specific language governing permissions and
; limitations under the License.

align 16

gdt:
	null_descriptor:
		dw 0x0000
		dw 0x0000
		db 0x00
		db 00000000b
		db 00000000b
		db 0x00

	kernel_code_64:
		dw 0x0000
		dw 0x0000
		db 0x00
		db 10011010b
		db 00100000b
		db 0x00

	kernel_data:
		dw 0x0000
		dw 0x0000
		db 0x00
		db 10010010b
		db 00000000b
		db 0x00

	user_data_64:
		dw 0x0000
		dw 0x0000
		db 0x00
		db 11110010b
		db 00000000b
		db 0x00

gdt_end:

global gdt_ptr

gdt_ptr:
	dw gdt_end - gdt - 1
	dq gdt

CODE_SEG equ kernel_code_64 - gdt
DATA_SEG equ kernel_data - gdt

global init_gdt

init_gdt:
	lgdt [gdt_ptr]
	mov rax, rsp
	push DATA_SEG
	push rax
	pushfq
	push CODE_SEG
	push flush
	iretq

flush:
	mov ax, DATA_SEG
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	ret
