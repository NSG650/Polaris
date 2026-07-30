#![allow(unsafe_op_in_unsafe_fn)]

use core::arch::naked_asm;
use core::ffi::c_int;

#[inline(never)]
#[unsafe(no_mangle)]
pub unsafe extern "C" fn strlen(str: *const u8) -> usize {
    let mut cur = str;
    while cur.read_volatile() != 0 {
        cur = cur.add(1);
    }
    cur.offset_from_unsigned(str)
}

#[unsafe(naked)]
#[unsafe(no_mangle)]
pub unsafe extern "C" fn memset(dest: *mut u8, val: c_int, size: usize) -> *mut u8 {
    naked_asm!(
        "push rdi",
        "mov rax, rsi",
        "mov rcx, rdx",
        "rep stosb",
        "pop rax",
        "ret"
    )
}

#[unsafe(naked)]
#[unsafe(no_mangle)]
pub unsafe extern "C" fn memcmp(_a: *const i8, _b: *const i8, _size: usize) -> i32 {
    naked_asm!(
        "mov rcx, rdx",
        "repe cmpsb",
        "je eqf",
        "mov al, byte ptr [rdi-1]",
        "sub al, byte ptr [rsi-1]",
        "movsx rax, al",
        "jmp donecmpf",
        "eq:",
        "xor eax, eax",
        "donecmp:",
        "ret"
    )
}

#[unsafe(naked)]
#[unsafe(no_mangle)]
pub unsafe extern "C" fn memcpy(dest: *mut u8, src: *const u8, size: usize) -> *mut u8 {
    naked_asm!("mov rcx, rdx", "mov rax, rdi", "rep movsb", "ret")
}

#[unsafe(naked)]
#[unsafe(no_mangle)]
pub unsafe extern "C" fn memmove(dest: *mut u8, src: *const u8, size: usize) -> *mut u8 {
    naked_asm!(
        "mov rcx, rdx",
        "mov rax, rdi",
        "cmp rdi, rsi",
        "ja copy_backwardsf",
        "rep movsb",
        "jmp donemovef",
        "copy_backwards:",
        "lea rdi, [rdi+rcx-1]",
        "lea rsi, [rsi+rcx-1]",
        "std",
        "rep movsb",
        "cld",
        "donemove:",
        "ret"
    )
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn __stack_chk_fail() -> !{
    loop
    {
        core::hint::spin_loop();
    }
}