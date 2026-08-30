use crate::arch::*;
use crate::locks::spinlock::SpinLock;
use alloc::sync::Arc;
use core::sync::atomic::AtomicU64;
use spin::Once;

pub static KERNEL_ADDRESS_SPACE: Once<Arc<SpinLock<AddressSpace>>> = Once::new();
pub static HHDM_OFFSET: AtomicU64 = AtomicU64::new(0);

pub(crate) struct PageTable {
    pub(crate) directory: u64,
}

pub struct AddressSpace {
    pub(crate) page_table: PageTable,
}

impl AddressSpace {
    pub fn new() -> Self {
        Self {
            page_table: PageTable::new(),
        }
    }

    pub fn map(&mut self, virt: u64, phys: u64, flags: PteFlags) {
        self.page_table.map(virt, phys, flags);
    }

    pub fn unmap(&mut self, virt: u64) -> Option<u64> {
        self.page_table.unmap(virt)
    }

    pub unsafe fn set(&self) {
        self.page_table.set();
    }
}
