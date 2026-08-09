#[cfg(target_arch = "x86_64")]
pub mod x86_64;
pub use crate::arch::x86_64::hpet as time;
pub use crate::arch::x86_64::intr;
pub use crate::arch::x86_64::smp::*;
pub use crate::arch::x86_64::*;
