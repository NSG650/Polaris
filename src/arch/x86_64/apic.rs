use core::sync::atomic::{AtomicBool, AtomicU64, Ordering};

use super::acpi;
use super::acpi::AcpiHeader;
use super::asm::{cpuid, outb, rdmsr, wrmsr};
use crate::mm::virt::HHDM_OFFSET;

const MADT_TYPE_PROCESSOR_LOCAL_APIC: u8 = 0;
const MADT_TYPE_NON_MASKABLE_INTERRUPT: u8 = 4;
const MADT_TYPE_LOCAL_APIC_ADDRESS_OVERRIDE: u8 = 5;

#[repr(C, packed)]
pub struct MadtHeader {
    pub entry_type: u8,
    pub record_length: u8,
}

#[repr(C, packed)]
pub struct Madt {
    pub header: AcpiHeader,
    pub local_apic_address: u32,
    pub flags: u32,
    pub entry0: MadtHeader,
}

#[repr(C, packed)]
pub struct MadtProcessorLocalApic {
    pub header: MadtHeader,
    pub acpi_processor_id: u8,
    pub apic_id: u8,
    pub flags: u32,
}

#[repr(C, packed)]
pub struct MadtNmi {
    pub header: MadtHeader,
    pub acpi_processor_id: u8,
    pub flags: u16,
    pub lint: u8,
}

static MADT_PTR: AtomicU64 = AtomicU64::new(0);

const MAX_MADT_ENTRIES: usize = 256;
static mut MADT_LOCAL_APICS: [*const MadtProcessorLocalApic; MAX_MADT_ENTRIES] =
    [core::ptr::null(); MAX_MADT_ENTRIES];
static mut MADT_NMIS: [*const MadtNmi; MAX_MADT_ENTRIES] = [core::ptr::null(); MAX_MADT_ENTRIES];

pub static MADT_LOCAL_APIC_COUNT: AtomicU64 = AtomicU64::new(0);
static MADT_NMI_COUNT: AtomicU64 = AtomicU64::new(0);

static X2APIC: AtomicBool = AtomicBool::new(false);
pub static BSP_LOCAL_APIC_ID: AtomicU64 = AtomicU64::new(0);
static LAPIC_ADDR: AtomicU64 = AtomicU64::new(0);

pub fn init() {
    MADT_PTR.store(
        acpi::find_sdt(b"APIC", 0).expect("Did not find APIC table?") as u64,
        Ordering::Relaxed,
    );

    let madt_ptr = MADT_PTR.load(Ordering::Relaxed) as *const Madt;
    let madt_len = unsafe { (*madt_ptr).header.length } as u64;
    let entry0_addr = unsafe { &(*madt_ptr).entry0 as *const MadtHeader as u64 };
    let madt_end = madt_ptr as u64 + madt_len;

    let mut local_apic_count = 0u64;
    let mut nmi_count = 0u64;

    let mut cursor = entry0_addr;
    while cursor < madt_end {
        let entry = unsafe { &*(cursor as *const MadtHeader) };
        match entry.entry_type {
            MADT_TYPE_PROCESSOR_LOCAL_APIC => {
                unsafe {
                    MADT_LOCAL_APICS[local_apic_count as usize] =
                        cursor as *const MadtProcessorLocalApic;
                }
                local_apic_count += 1;
            }
            MADT_TYPE_NON_MASKABLE_INTERRUPT => {
                unsafe {
                    MADT_NMIS[nmi_count as usize] = cursor as *const MadtNmi;
                }
                nmi_count += 1;
            }
            _ => {}
        }
        cursor += entry.record_length as u64;
    }

    MADT_LOCAL_APIC_COUNT.store(local_apic_count, Ordering::Relaxed);
    MADT_NMI_COUNT.store(nmi_count, Ordering::Relaxed);

    LAPIC_ADDR.store(
        0xfee00000 + HHDM_OFFSET.load(Ordering::Relaxed),
        Ordering::Relaxed,
    );

    let cpuid1 = cpuid(1, 0);
    if cpuid1.ecx & (1 << 21) != 0 {
        X2APIC.store(true, Ordering::Relaxed);
    }

    let madt_flags = unsafe { (*madt_ptr).flags };
    if madt_flags & 1 != 0 {
        // Remap the legacy PIC, then mask it off entirely.
        unsafe {
            outb(0x20, 0x11);
            outb(0xA0, 0x11);
            outb(0x21, 0x20);
            outb(0xA1, 0x28);
            outb(0x21, 4);
            outb(0xA1, 2);
            outb(0x21, 1);
            outb(0xA1, 1);
            outb(0x21, 0);
            outb(0xA1, 0);

            // Disable the PIC
            outb(0xA1, 0xFF);
            outb(0x21, 0xFF);
        }
    }
}

fn reg_to_x2apic(reg: u32) -> u32 {
    let x2apic_reg = if reg == 0x310 { 0x30 } else { reg >> 4 };
    x2apic_reg + 0x800
}

pub struct Lapic;

impl Lapic {
    pub fn read(reg: u32) -> u32 {
        if X2APIC.load(Ordering::Relaxed) {
            unsafe { rdmsr(reg_to_x2apic(reg)) as u32 }
        } else {
            let addr = LAPIC_ADDR.load(Ordering::Relaxed) + reg as u64;
            unsafe { core::ptr::read_volatile(addr as *const u32) }
        }
    }

    pub fn write(reg: u32, value: u32) {
        if X2APIC.load(Ordering::Relaxed) {
            unsafe { wrmsr(reg_to_x2apic(reg), value as u64) };
        } else {
            let addr = LAPIC_ADDR.load(Ordering::Relaxed) + reg as u64;
            unsafe { core::ptr::write_volatile(addr as *mut u32, value) };
        }
    }

    pub fn get_id() -> u8 {
        (Self::read(0x20) >> 24) as u8
    }

    fn set_nmi(vec: u8, current_processor_id: u8, processor_id: u8, flags: u16, lint: u8) {
        if processor_id != 0xFF && current_processor_id != processor_id {
            return;
        }

        let mut nmi: u32 = 0x400 | vec as u32;
        if flags & 2 != 0 {
            nmi |= 1 << 13; // active low
        }
        if flags & 8 != 0 {
            nmi |= 1 << 15; // level triggered
        }

        match lint {
            0 => Self::write(0x350, nmi),
            1 => Self::write(0x360, nmi),
            _ => {}
        }
    }

    pub fn eoi() {
        Self::write(0xB0, 0);
    }

    pub fn init(processor_id: u8) {
        unsafe {
            wrmsr(0x1b, (1 << 11) | rdmsr(0x1b));
            if X2APIC.load(Ordering::Relaxed) {
                wrmsr(0x1b, (1 << 10) | rdmsr(0x1b));
            }
        }

        Self::write(0x80, 0);
        Self::write(0xF0, Self::read(0xF0) | 0x100);

        if !X2APIC.load(Ordering::Relaxed) {
            Self::write(0xE0, 0xF0000000);
            Self::write(0xD0, Self::read(0x20));
        }

        let nmi_count = MADT_NMI_COUNT.load(Ordering::Relaxed);
        for i in 0..nmi_count {
            let nmi = unsafe { &*MADT_NMIS[i as usize] };
            Self::set_nmi(2, processor_id, nmi.acpi_processor_id, nmi.flags, nmi.lint);
        }
    }

    pub fn send_ipi(lapic_id: u32, flags: u32) {
        if X2APIC.load(Ordering::Relaxed) {
            unsafe { wrmsr(0x830, ((lapic_id as u64) << 32) | flags as u64) };
        } else {
            Self::write(0x310, lapic_id << 24);
            Self::write(0x300, flags);
        }
    }

    pub fn send_ipi_to(processor: u32, vector: u32) {
        Self::send_ipi(processor, vector);
    }

    pub fn broadcast_ipi(vector: u32, include_me: bool) {
        if include_me {
            Self::send_ipi(0, (2 << 18) | vector);
        } else {
            Self::send_ipi(0, (3 << 18) | vector);
        }
    }
}
