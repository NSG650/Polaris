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
mod object;
mod sched;

use crate::{
    locks::mutex::Mutex,
    object::handle::Handle,
    sched::{
        dispatch::{DispatcherObject, Event},
        thread::Thread,
    },
};
use spin::Once;

use alloc::sync::Arc;

#[macro_use]
mod log;

#[unsafe(no_mangle)]
unsafe extern "C" fn _start() {
    arch::entry::arch_entry();
}

extern "C" fn another_thread(arg: usize) -> ! {
    log!("Hello from another thread!\r\n");

    let dispatcher_object = arch::get_running_thread()
        .unwrap()
        .mother_proc
        .handle_table
        .lock()
        .get(arg)
        .expect("We should've gotten a handle??")
        .get();

    let event: Arc<Event> = dispatcher_object.as_event().unwrap();

    event.trigger(true);

    log!("Triggered the event!\r\n");

    arch::get_running_thread().unwrap().terminate();
    unreachable!()
}

extern "C" fn init_thread(_arg: usize) -> ! {
    log!("Hello from kernel init thread!\r\n");

    let event = Arc::new(Event::new());
    let devent: Arc<DispatcherObject> = Arc::new(event.clone().into());
    let handle = Handle::new(devent.clone());

    let handle_id = arch::get_running_thread()
        .unwrap()
        .mother_proc
        .handle_table
        .lock()
        .insert(handle);

    sched::sched::enqueue_thread(
        Thread::new_kernel(
            another_thread,
            handle_id,
            arch::get_running_thread().unwrap().mother_proc.clone(),
        )
        .unwrap(),
    );

    log!("Waiting on event to be triggered\r\n");
    event.trigger(false);

    sched::dispatch::wait_on_single_object(devent.clone(), usize::MAX);

    log!("The event was triggered!\r\n");

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
