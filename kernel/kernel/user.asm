global user_switch

user_switch:
    mov rcx, rdi
    mov r11, 0x202
    o64 sysret

