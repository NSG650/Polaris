; Copyright 2021, 2022 NSG650
; Copyright 2021, 2022 Sebastian
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

%include "kernel/cpu/stackop.inc"

extern isr_handler

; Common handler for the ISRs
isr_common_format:
	pushall
	cld
	mov rdi, rsp
	call isr_handler
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

%define has_errcode(i) (i == 8 || (i >= 10 && i <= 14) || i == 17 || i == 21)

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
