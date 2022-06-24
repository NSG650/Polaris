; Taken from Limine
; Added memset/cpy(16/32/64)
section .text

global memcpy
memcpy:
    mov rcx, rdx
    mov rax, rdi
    rep movsb
    ret
    
global memcpy16
memcpy16:
    mov rcx, rdx
    mov rax, rdi
    rep movsw
    ret

global memcpy32
memcpy32:
    mov rcx, rdx
    mov rax, rdi
    rep movsd
    ret
    
global memcpy64
memcpy64:
    mov rcx, rdx
    mov rax, rdi
    rep movsq
    ret

global memset
memset:
    push rdi
    mov rax, rsi
    mov rcx, rdx
    rep stosb
    pop rax
    ret

global memset16
memset16:
    push rdi
    mov rax, rsi
    mov rcx, rdx
    rep stosw
    pop rax
    ret
    
global memset32
memset32:
    push rdi
    mov rax, rsi
    mov rcx, rdx
    rep stosd
    pop rax
    ret
    
global memset64
memset64:
    push rdi
    mov rax, rsi
    mov rcx, rdx
    rep stosq
    pop rax
    ret

global memmove
memmove:
    mov rcx, rdx
    mov rax, rdi

    cmp rdi, rsi
    ja .copy_backwards

    rep movsb
    jmp .done

  .copy_backwards:
    lea rdi, [rdi+rcx-1]
    lea rsi, [rsi+rcx-1]
    std
    rep movsb
    cld

  .done:
    ret

global memcmp
memcmp:
    mov rcx, rdx
    repe cmpsb
    jecxz .equal

    mov al, byte [rdi-1]
    sub al, byte [rsi-1]
    movsx rax, al
    jmp .done

  .equal:
    xor eax, eax

  .done:
    ret
