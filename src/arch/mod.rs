use bitflags::bitflags;

pub mod intr;
#[cfg(target_arch = "x86_64")]
pub mod x86_64;
pub const PAGE_SIZE: usize = 4096;
pub const PAGE_SHIFT: usize = 12;
#[cfg(target_arch = "x86_64")]
pub const PFN_DATABASE: u64 = 0xFFFF_FA80_0000_0000;
pub const BIG_ALLOC_START: u64 = PFN_DATABASE + (1 << 40);
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
