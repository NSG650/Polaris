use super::apic;
use super::gdt;
use super::idt;
use crate::arch::x86_64::gdt::CpuGdt;
use crate::log;
use crate::mm::stack::*;
use crate::mm::virt::*;
use alloc::boxed::Box;
use core::array;
use core::sync::atomic::{AtomicUsize, Ordering};
use limine::mp::*;

const KERNEL_STACKS_COUNT: usize = 4;

pub struct Prcb {
    me: *mut Prcb,
    pub cpu_id: u32,
    gdt: gdt::CpuGdt,
    kernel_stacks: [KernelStack; KERNEL_STACKS_COUNT],
}

pub static PROCESSOR_COUNT: AtomicUsize = AtomicUsize::new(0);

unsafe extern "C" fn processor_startup(mp_info: &MpInfo) -> ! {
    unsafe {
        KERNEL_ADDRESS_SPACE.lock().as_ref().unwrap().set();
    }
    let prcb = mp_info.extra_argument() as *mut Prcb;
    let mut prcb = unsafe { &mut *prcb };

    prcb.gdt.load();
    idt::load();
    apic::Lapic::init(prcb.cpu_id as u8);

    log!("Hello I am processor {}\r\n", prcb.cpu_id);
    PROCESSOR_COUNT.fetch_add(1, Ordering::SeqCst);

    loop {}
}

fn processor_setup_bsp(prcb: &mut Prcb) {
    prcb.gdt.load();
    idt::load();
    apic::Lapic::init(prcb.cpu_id as u8);

    log!("Hello I am the boot processor {}\r\n", prcb.cpu_id);
    PROCESSOR_COUNT.fetch_add(1, Ordering::SeqCst);
}

pub(in crate::arch::x86_64) fn init(mp_request: &MpRespData) {
    for cpu in mp_request.cpus() {
        let mut kernel_stacks: [KernelStack; 4] =
            array::from_fn(|_| KernelStack::new().expect("Failed to allocate kernel stack?"));

        let mut prcb = Box::new(Prcb {
            me: core::ptr::null_mut(),
            cpu_id: cpu.processor_id,
            gdt: CpuGdt::new(),
            kernel_stacks,
        });

        let prcb_ref: &'static mut Prcb = Box::leak(prcb);
        prcb_ref.me = prcb_ref as *mut Prcb;

        prcb_ref.gdt.tss.rsp[0] = prcb_ref.kernel_stacks[0].top() as u64;

        prcb_ref.gdt.tss.ist[0] = prcb_ref.kernel_stacks[1].top() as u64;
        prcb_ref.gdt.tss.ist[1] = prcb_ref.kernel_stacks[2].top() as u64;
        prcb_ref.gdt.tss.ist[2] = prcb_ref.kernel_stacks[3].top() as u64;

        if cpu.lapic_id == mp_request.bsp_lapic_id {
            processor_setup_bsp(prcb_ref);
        } else {
            cpu.bootstrap(processor_startup, prcb_ref.me as u64);
        }
    }

    while PROCESSOR_COUNT.load(Ordering::Relaxed) != mp_request.cpus().len() {}

    log!(
        "The system has {} processor(s) installed\r\n",
        PROCESSOR_COUNT.load(Ordering::Relaxed)
    );
}
