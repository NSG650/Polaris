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
mod sched;

#[macro_use]
mod log;

#[unsafe(no_mangle)]
unsafe extern "C" fn _start() {
    arch::entry::arch_entry();
}

extern "C" fn init_thread(arg: usize) -> ! {
    log!("Hello from kernel init thread!\r\n");
    loop {}
}

#[panic_handler]
fn panic_handler(info: &PanicInfo) -> ! {
    log!("*** PANIC!\r\n");
    if let Some(loc) = info.location() {
        log!("PANIC: {}:{}: ", loc.file(), loc.line());
    }
    log!("{}\r\n", info.message());
    loop {
        arch::asm::halt_forever();
    }
}
