use crate::arch::{PAGE_SIZE, PteFlags, STACK_ALLOCATIONS_START, STACK_SIZE};
use crate::mm::phys::*;
use crate::mm::virt::*;
use core::sync::atomic::{AtomicU64, Ordering};

static STACK_ALLOCATION_PTR: AtomicU64 = AtomicU64::new(STACK_ALLOCATIONS_START);

pub struct KernelStack {
    base: usize,
    size: usize,
}

impl KernelStack {
    pub fn new() -> Option<Self> {
        let mut ptr = STACK_ALLOCATION_PTR
            .fetch_add((STACK_SIZE + PAGE_SIZE) as u64, Ordering::Relaxed)
            + PAGE_SIZE as u64;

        let mut kernel_address_space = KERNEL_ADDRESS_SPACE.lock();
        let kernel_address_space = kernel_address_space.as_mut().unwrap();

        for i in (0..STACK_SIZE).step_by(PAGE_SIZE) {
            let page = match PMM.lock().as_mut().unwrap().alloc(PageUsage::KernelStack) {
                Some(p) => p,
                None => return None,
            };

            kernel_address_space.map(ptr + i as u64, page, PteFlags::PRESENT | PteFlags::WRITABLE);
        }

        Some(Self {
            base: ptr as usize,
            size: STACK_SIZE,
        })
    }

    pub fn top(&self) -> usize {
        self.base + self.size
    }
}

impl Drop for KernelStack {
    fn drop(&mut self) {
        let mut kernel_address_space = KERNEL_ADDRESS_SPACE.lock();
        let kernel_address_space = kernel_address_space.as_mut().unwrap();

        for i in (0..self.size).step_by(PAGE_SIZE) {
            if let Some(phys) = kernel_address_space.unmap((self.base + i) as u64) {
                PMM.lock().as_mut().unwrap().free(phys);
            }
        }
    }
}
