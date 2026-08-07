use bitflags::bitflags;

#[cfg(target_arch = "x86_64")]
pub mod x86_64;
pub use crate::arch::x86_64::intr;
pub const PAGE_SIZE: usize = 4096;
pub const PAGE_SHIFT: usize = 12;
pub const STACK_SIZE: usize = PAGE_SIZE * 16;
pub const PFN_DATABASE: u64 = 0xFFFF_FA80_0000_0000;
pub const BIG_ALLOC_START: u64 = PFN_DATABASE + (1 << 40);
pub const STACK_ALLOCATIONS_START: u64 = PFN_DATABASE + (2 << 40);
bitflags! {
    #[derive(Clone, Copy)]
    pub struct PteFlags: u64 {
        const PRESENT     = 1 << 0;
        const WRITABLE    = 1 << 1;
        const USER        = 1 << 2;
        const HUGE        = 1 << 7;
        const NO_EXECUTE  = 1 << 63;
    }
}
