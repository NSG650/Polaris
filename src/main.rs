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

use crate::{locks::mutex::Mutex, sched::thread::Thread};
use spin::Once;

#[macro_use]
mod log;

#[unsafe(no_mangle)]
unsafe extern "C" fn _start() {
    arch::entry::arch_entry();
}

static TEST_MUTEX: Once<Mutex<u64>> = Once::new();
const THREADS: u64 = 4;
const ITERS_PER_THREAD: u64 = 100_000;

extern "C" fn mutex_worker(_: usize) -> ! {
    log!("Hello I am a mutex worker thread!\r\n");
    let mutex = TEST_MUTEX.get().unwrap();

    for _ in 0..ITERS_PER_THREAD {
        let mut guard = mutex.lock();
        let val = *guard;
        *guard = val + 1;
    }

    log!("Done\r\n");
    Thread::terminate(arch::get_running_thread().unwrap());
    unreachable!()
}

extern "C" fn init_thread(_arg: usize) -> ! {
    log!("Hello from kernel init thread!\r\n");
    TEST_MUTEX.call_once(|| Mutex::new(0u64));

    for _ in 0..THREADS {
        sched::sched::enqueue_thread(
            Thread::new_kernel(mutex_worker, 0, sched::process::kernel_process().clone())
                .expect("failed to create mutex worker thread"),
        );
    }

    loop {
        let val = *TEST_MUTEX.get().unwrap().lock();
        if val == THREADS * ITERS_PER_THREAD {
            log!("Test mutex value is {}\r\n", val);
            loop {}
        }
        sched::sched::yield_execution();
    }
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
