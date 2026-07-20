#![no_std]
#![no_main]
#![feature(formatting_options)]

use core::panic::PanicInfo;

extern crate alloc;
extern crate core;
mod arch;
mod fbcon;
mod locks;
mod mm;

#[macro_use]
mod log;

#[unsafe(no_mangle)]
unsafe extern "C" fn _start() {
    #[cfg(target_arch = "x86_64")]
    arch::x86_64::entry::arch_entry();
}

#[panic_handler]
fn panic_handler(info: &PanicInfo) -> ! {
    log!("*** PANIC!\r\n");
    if let Some(loc) = info.location() {
        log!("PANIC: {}:{}: ", loc.file(), loc.line());
    }
    log!("{}\r\n", info.message());
    loop {
        #[cfg(target_arch = "x86_64")]
        arch::x86_64::asm::halt_forever();
    }
}
