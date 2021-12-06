; Copyright 2021 NSG650
; Copyright 2021 Sebastian
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

; Define ISRs
isr 0
isr 1
isr 2
isr 3
isr 4
isr 5
isr 6
isr 7
error_isr 8
isr 9
error_isr 10
error_isr 11
error_isr 12
error_isr 13
error_isr 14
%assign i 15
%rep 241
	isr i
%assign i i + 1
%endrep
