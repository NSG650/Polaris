#[cfg(target_arch = "x86_64")]
use super::x86_64;

pub unsafe fn enable_interrupts() {
    unsafe {
        #[cfg(target_arch = "x86_64")]
        x86_64::asm::sti();
    }
}

pub unsafe fn disable_interrupts() {
    unsafe {
        #[cfg(target_arch = "x86_64")]
        x86_64::asm::cli();
    }
}

pub unsafe fn get_interrupt_state() -> bool {
    unsafe {
        #[cfg(target_arch = "x86_64")]
        return x86_64::asm::interrupt_state();
    }
}

pub unsafe fn toggle_interrupts(state: bool) -> bool {
    unsafe {
        #[cfg(target_arch = "x86_64")]
        return x86_64::asm::toggle_interrupts(state);
    }
}
