use super::asm;
use seq_macro;

pub unsafe fn enable_interrupts() {
    unsafe {
        asm::sti();
    }
}

pub unsafe fn disable_interrupts() {
    unsafe {
        asm::cli();
    }
}

pub unsafe fn get_interrupt_state() -> bool {
    unsafe {
        return asm::interrupt_state();
    }
}

pub unsafe fn toggle_interrupts(state: bool) -> bool {
    unsafe {
        return asm::toggle_interrupts(state);
    }
}

use core::arch::naked_asm;

#[repr(C)]
pub struct Context {
    pub r15: u64,
    pub r14: u64,
    pub r13: u64,
    pub r12: u64,
    pub r11: u64,
    pub r10: u64,
    pub r9: u64,
    pub r8: u64,
    pub rsi: u64,
    pub rdi: u64,
    pub rbp: u64,
    pub rdx: u64,
    pub rcx: u64,
    pub rbx: u64,
    pub rax: u64,
    pub isr: u64,
    pub error: u64,
    pub rip: u64,
    pub cs: u64,
    pub rflags: u64,
    pub rsp: u64,
    pub ss: u64,
}

const HAS_ERROR_CODE: fn(u64) -> bool =
    |i| i == 8 || (i >= 10 && i <= 14) || i == 17 || i == 21 || i == 29 || i == 30;

seq_macro::seq!(N in 0..256 {
    #[unsafe(naked)]
    pub unsafe extern "C" fn interrupt_stub~N() {
        naked_asm!(
            ".if ({i} == 8 || ({i} >= 10 && {i} <= 14) || {i} == 17 || {i} == 21 || {i} == 29 || {i} == 30)",
            ".else",
            "push 0",
            ".endif",
            "push {i}",
            "jmp {internal}",
            i = const N,
            internal = sym interrupt_stub_internal,
        );
    }
});

#[unsafe(naked)]
unsafe extern "C" fn interrupt_stub_internal() {
    naked_asm!(
        "push rax",
        "push rbx",
        "push rcx",
        "push rdx",
        "push rbp",
        "push rdi",
        "push rsi",
        "push r8",
        "push r9",
        "push r10",
        "push r11",
        "push r12",
        "push r13",
        "push r14",
        "push r15",
        "cld",
        "xor rbp, rbp",
        "mov rdi, rsp",
        "call {handler}",
        "jmp {ret}",
        handler = sym idt_handler,
        ret = sym interrupt_return,
    );
}

#[unsafe(naked)]
pub unsafe extern "C" fn interrupt_return() {
    naked_asm!(
        "pop r15",
        "pop r14",
        "pop r13",
        "pop r12",
        "pop r11",
        "pop r10",
        "pop r9",
        "pop r8",
        "pop rsi",
        "pop rdi",
        "pop rbp",
        "pop rdx",
        "pop rcx",
        "pop rbx",
        "pop rax",
        "add rsp, 0x10",
        "iretq",
    );
}

pub unsafe extern "C" fn idt_handler(context: *mut Context) {
    let context = unsafe { &mut *context };
    let isr = context.isr as u8;

    match isr {
        0x00..0x1F => {
            panic!("Got kernel exception {}: {:#x?}", isr, context.error);
        }
        _ => {
            todo!()
        }
    }
}
