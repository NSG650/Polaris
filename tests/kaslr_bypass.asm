BITS 64

section .text

global side_channel
side_channel:
    xor rax, rax
    xor rdx, rdx

    xor r10, r10
    xor r11, r11
    xor r12, r12

    mov r12, rdi                    

    mfence

    rdtscp                          ; We use rdtscp instead of rdtsc since rdtscp waits for previous instructions to be completed before it runs.

    mov r10, rax
    mov r11, rdx

    shl r11, 32
    or r11, r10

    lfence

    prefetchnta [r12]
    prefetcht2 [r12]

    mfence

    rdtscp

    shl rdx, 32
    or rdx, rax

    lfence

    sub rdx, r11
    mov rax, rdx

    ret
    