#![allow(unsafe_op_in_unsafe_fn)]

use core::arch::naked_asm;
use core::ffi::c_int;
use core::ffi::c_void;

#[inline(never)]
#[unsafe(no_mangle)]
pub unsafe extern "C" fn strlen(str: *const i8) -> usize {
    let mut cur = str;
    while cur.read_volatile() != 0 {
        cur = cur.add(1);
    }
    cur.offset_from_unsigned(str)
}

#[unsafe(naked)]
#[unsafe(no_mangle)]
pub unsafe extern "C" fn memset(dest: *mut c_void, val: c_int, size: usize) -> *mut c_void {
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
pub unsafe extern "C" fn memcmp(_a: *const c_void, _b: *const c_void, _size: usize) -> i32 {
    naked_asm!(
        "mov rcx, rdx",
        "repe cmpsb",
        "je 2f",
        "mov al, byte ptr [rdi-1]",
        "sub al, byte ptr [rsi-1]",
        "movsx eax, al",
        "jmp 3f",
        "2:",
        "xor eax, eax",
        "3:",
        "ret"
    )
}

#[unsafe(naked)]
#[unsafe(no_mangle)]
pub unsafe extern "C" fn memcpy(dest: *mut c_void, src: *const c_void, size: usize) -> *mut c_void {
    naked_asm!("mov rcx, rdx", "mov rax, rdi", "rep movsb", "ret")
}

#[unsafe(naked)]
#[unsafe(no_mangle)]
pub unsafe extern "C" fn memmove(
    dest: *mut c_void,
    src: *const c_void,
    size: usize,
) -> *mut c_void {
    naked_asm!(
        "mov rcx, rdx",
        "mov rax, rdi",
        "cmp rdi, rsi",
        "ja copy_backwardsf",
        "rep movsb",
        "jmp donemovef",
        "copy_backwardsf:",
        "lea rdi, [rdi+rcx-1]",
        "lea rsi, [rsi+rcx-1]",
        "std",
        "rep movsb",
        "cld",
        "donemovef:",
        "ret"
    )
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn __stack_chk_fail() -> ! {
    panic!("__stack_chk_fail");
    loop {}
}
