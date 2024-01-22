; Copyright 2021 - 2023 NSG650
; Copyright 2021 - 2023 Neptune
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

%macro pushall 0

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

%endmacro

%macro popall 0

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

%endmacro

extern isr_handle

; Common handler for the ISRs
isr_common_format:
	pushall
	cld
	mov rdi, rsp
	xor rbp, rbp
	call isr_handle
	popall
	add rsp, 24
	iretq

%macro isr 1

global isr%1
isr%1:
	push 0
	push %1
	push fs
	jmp isr_common_format

%endmacro

%macro error_isr 1

global isr%1
isr%1:
	push %1
	push fs
	jmp isr_common_format

%endmacro

%define has_errcode(i) (i == 8 || (i >= 10 && i <= 14) || i == 17 || i == 21 || i == 29 || i == 30)

; Define ISRs
%assign i 0
%rep 256
%if !has_errcode(i)
	isr i
%else
	error_isr i
%endif
%assign i i + 1
%endrep
