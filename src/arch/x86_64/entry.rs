use super::acpi;
use super::apic;
use super::asm;
use super::hpet;
use super::idt;
use super::intr;
use super::mminit;
use super::smp;
use crate::arch;
use crate::fbcon;
use crate::log;
use crate::sched::dispatch::{self, DispatcherObject, Event};
use crate::sched::sched;
use crate::sched::thread::Thread;
use alloc::sync::Arc;
use alloc::vec::Vec;
use core::sync::atomic::Ordering;
use flanterm;
use limine::{BaseRevision, RequestsEndMarker, RequestsStartMarker, request::*};

#[used]
#[unsafe(link_section = ".requests_start")]
pub static REQUESTS_START: RequestsStartMarker = RequestsStartMarker::new();
#[unsafe(link_section = ".requests")]
pub static BASE_REVISION: BaseRevision = BaseRevision::new();
#[unsafe(link_section = ".requests")]
pub static STACK: StackSizeRequest = StackSizeRequest::new(65536);
#[unsafe(link_section = ".requests")]
static FRAMEBUFFER: FramebufferRequest = FramebufferRequest::new();
#[unsafe(link_section = ".requests")]
pub static MEMMAP: MemmapRequest = MemmapRequest::new();
#[unsafe(link_section = ".requests")]
pub static HHDM: HhdmRequest = HhdmRequest::new();
#[unsafe(link_section = ".requests")]
pub static EXEC_ADDR: ExecutableAddressRequest = ExecutableAddressRequest::new();
#[unsafe(link_section = ".requests")]
pub static RSDP: RsdpRequest = RsdpRequest::new();
#[unsafe(link_section = ".requests")]
pub static MP_REQUEST: MpRequest = MpRequest::new(1);
#[used]
#[unsafe(link_section = ".requests_end")]
pub static REQUESTS_END: RequestsEndMarker = RequestsEndMarker::new();

extern "C" fn test_thread(arg: usize) {
    log!("Hello I am test thread {}\r\n", arg);

    arch::get_running_thread().unwrap().terminate();
}

extern "C" fn thread0(arg: usize) {
    const THREAD_COUNT: usize = 10;
    log!(
        "Hello I am thread0 and I am going to spawn {} threads\r\n",
        THREAD_COUNT
    );

    let mut threads = Vec::with_capacity(THREAD_COUNT);
    for i in 0..THREAD_COUNT {
        let ktest_thread = Arc::new(Thread::new_kernel(test_thread, i).unwrap());
        let dtest_thread: Arc<DispatcherObject> = Arc::new(ktest_thread.clone().into());
        sched::enqueue_thread(ktest_thread);

        threads.push(dtest_thread);
    }

    dispatch::wait_on_multiple_objects(&threads, true, usize::MAX);

    log!("Test threads got terminated\r\n");

    loop {}
}

pub fn arch_entry() {
    if let Some(resp) = FRAMEBUFFER.response()
        && let Some(fb) = resp.framebuffers().first()
    {
        unsafe {
            fbcon::fbcon_init(
                fb.address(),
                fb.width as usize,
                fb.height as usize,
                fb.pitch as usize,
                fb.red_mask_size,
                fb.red_mask_shift,
                fb.green_mask_size,
                fb.green_mask_shift,
                fb.blue_mask_size,
                fb.blue_mask_shift,
                flanterm::fb::Rotation::Rot0,
            );
        }
    }

    log!("Hello x86_64!\r\n");

    mminit::init(
        MEMMAP
            .response()
            .expect("Did not get a memory map??")
            .entries(),
        HHDM.response().expect("Did not get HHDM??").offset,
        EXEC_ADDR
            .response()
            .expect("Did not get the executable address??")
            .physical_base,
        EXEC_ADDR
            .response()
            .expect("Did not get the executable address??")
            .virtual_base,
    );

    idt::init();

    acpi::init(RSDP.response().expect("Did not get RSDP pointer??").address);
    hpet::init();
    apic::init();
    smp::init(MP_REQUEST.response().expect("Did not get SMP response??"));

    sched::enqueue_thread(Arc::new(Thread::new_kernel(thread0, 0).unwrap()));

    unsafe {
        intr::enable_interrupts();
    }

    loop {
        asm::halt_forever();
    }
}
