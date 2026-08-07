use super::gdt::SEL_KERNEL_CODE;
use super::intr::*;
use core::arch::asm;

pub const IDT_SIZE: usize = 256;

#[repr(C, packed)]
struct IdtRegister {
    limit: u16,
    base: *const Idt,
}

#[repr(align(0x1000))]
pub struct Idt {
    entries: [IdtEntry; IDT_SIZE],
}

impl Idt {
    const fn new() -> Self {
        Self {
            entries: [IdtEntry::new(); IDT_SIZE],
        }
    }
}

#[repr(C, packed)]
#[derive(Clone, Copy)]
struct IdtEntry {
    base0: u16,
    selector: u16,
    ist: u8,
    attributes: u8,
    base1: u16,
    base2: u32,
    reserved: u32,
}

#[repr(u8)]
enum GateType {
    Interrupt = 0xE,
}

impl IdtEntry {
    const fn new() -> Self {
        Self {
            base0: 0,
            selector: 0,
            ist: 0,
            attributes: 0,
            base1: 0,
            base2: 0,
            reserved: 0,
        }
    }

    const fn init(base: u64, ist: u8, gate: GateType, dpl: u8) -> Self {
        Self {
            base0: base as u16,
            selector: SEL_KERNEL_CODE,
            ist,
            attributes: 0x80 | ((dpl & 0x3) << 5) | (gate as u8 & 0xF),
            base1: (base >> 16) as u16,
            base2: (base >> 32) as u32,
            reserved: 0,
        }
    }
}

static mut IDT_TABLE: Idt = Idt::new();

pub fn init() {
    unsafe {
        seq_macro::seq!(N in 0..256 {
            (*&raw mut IDT_TABLE).entries[N] =
                IdtEntry::init(interrupt_stub~N as usize as u64, 0, GateType::Interrupt, 0);
        });

        (*&raw mut IDT_TABLE).entries[8].ist = 1;
        (*&raw mut IDT_TABLE).entries[2].ist = 2;
    }
}

pub fn load() {
    let idtr = IdtRegister {
        limit: (size_of::<Idt>() - 1) as u16,
        base: &raw const IDT_TABLE,
    };
    unsafe {
        asm!("lidt [{0}]", in(reg) &idtr, options(readonly, nostack, preserves_flags));
    }
}
