[bits 64]

align 0x10

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
  retf
  flush:
  mov ax, DATA_SEG
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax
  ret

