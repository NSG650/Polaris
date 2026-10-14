use super::var;
use super::virt;
use crate::arch::{self, PAGE_SIZE};
use crate::log;
use crate::mm::phys::*;
use crate::mm::var::VarProtectionFlags;
use crate::mm::virt::HHDM_OFFSET;
use crate::mm::vmb;
use crate::sched::process;
use crate::sched::thread;
use crate::status_codes::{PxResult, PxStatus};
use bitflags::bitflags;
use core::sync::atomic::{AtomicUsize, Ordering};

fn align_down(x: u64, a: u64) -> u64 {
    x & !(a - 1)
}

bitflags! {
    #[derive(Clone, Copy, Eq, PartialEq)]
    pub struct FaultType: i32 {
        const READ     = 1 << 0;
        const WRITE    = 1 << 1;
    }
}

pub fn handle_fault(faulting_address: u64, fault_type: FaultType) -> PxResult<()> {
    let Some(running_thread) = arch::get_running_thread() else {
        return Err(PxStatus::Unsuccessful);
    };

    let mut addr_space = running_thread.mother_proc.address_space.lock();

    let Some((base_address, protections, vmb)) = addr_space
        .get_var_range(faulting_address)
        .map(|var| (var.base_address, var.protections, var.vmb.clone()))
    else {
        return Err(PxStatus::NotFound);
    };

    let page = match vmb.get_page((faulting_address - base_address) as usize) {
        Ok(p) => p,
        Err(status) => return Err(status),
    };

    let mut page_to_map = page;
    if protections.contains(VarProtectionFlags::COW) && fault_type.contains(FaultType::WRITE) {
        // crate::log!("Mooooooo {:X}\r\n", faulting_address);
        let Some(new_page) = PMM.lock().as_mut().unwrap().alloc(PageUsage::Anon) else {
            return Err(PxStatus::FailedToAllocate);
        };

        let new_ptr = (new_page + (HHDM_OFFSET.load(Ordering::Relaxed))) as *mut ();
        let page_ptr = (page + (HHDM_OFFSET.load(Ordering::Relaxed))) as *const ();

        unsafe {
            new_ptr.copy_from(page_ptr, PAGE_SIZE);
        }

        page_to_map = new_page;
    }

    addr_space.map(
        align_down(faulting_address, PAGE_SIZE as u64),
        page_to_map,
        arch::prot_to_pte_flags(if page_to_map != page {
            protections - VarProtectionFlags::COW
        } else {
            protections
        }),
    );

    Ok(())
}
