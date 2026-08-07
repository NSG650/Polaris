use super::asm;
use crate::log;
use core::fmt::Write;

impl Write for log::DebugCon {
    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        unsafe {
            for &c in s.as_bytes() {
                asm::outb(0xE9, c);
            }
        }
        Ok(())
    }
}
