use core::sync::atomic::{AtomicBool, AtomicU8, Ordering};

use crate::log;
use crate::mm::virt::HHDM_OFFSET;

#[repr(C, packed)]
pub struct AcpiHeader {
    pub signature: [u8; 4],
    pub length: u32,
    pub revision: u8,
    pub checksum: u8,
    pub oem_id: [u8; 6],
    pub oem_table_id: [u8; 8],
    pub oem_revision: u32,
    pub creator_id: u32,
    pub creator_revision: u32,
}

#[repr(C, packed)]
pub struct AcpiXsdp {
    pub signature: [u8; 8],
    pub checksum: u8,
    pub oem_id: [u8; 6],
    pub revision: u8,
    pub rsdt: u32,
    // ACPI 2.0+ fields:
    pub length: u32,
    pub xsdt: u64,
    pub extended_checksum: u8,
    pub reserved: [u8; 3],
}

#[repr(C, packed)]
pub struct Rsdt {
    pub header: AcpiHeader,
}

static USE_XSDT: AtomicBool = AtomicBool::new(false);
static RSDT_PTR: core::sync::atomic::AtomicU64 = core::sync::atomic::AtomicU64::new(0);
static REVISION: AtomicU8 = AtomicU8::new(0);

pub fn init(rsdp: *mut ()) {
    let rsdp = rsdp as *const AcpiXsdp;
    let rsdp = unsafe { &*rsdp };

    let revision = rsdp.revision;
    log!("ACPI Revision: {}\r\n", revision);
    REVISION.store(revision, Ordering::Relaxed);

    if revision >= 2 && rsdp.xsdt != 0 {
        USE_XSDT.store(true, Ordering::Relaxed);
        let addr = rsdp.xsdt + HHDM_OFFSET.load(Ordering::Relaxed);
        RSDT_PTR.store(addr, Ordering::Relaxed);
        log!("Using XSDT at {:#x}\r\n", addr);
    } else {
        USE_XSDT.store(false, Ordering::Relaxed);
        let addr = rsdp.rsdt as u64 + HHDM_OFFSET.load(Ordering::Relaxed);
        RSDT_PTR.store(addr, Ordering::Relaxed);
        log!("Using RSDT at {:#x}\r\n", addr);
    }
}

fn checksum(ptr: *const u8, size: usize) -> u8 {
    let mut sum: u8 = 0;
    for i in 0..size {
        sum = sum.wrapping_add(unsafe { *ptr.add(i) });
    }
    sum
}

pub fn find_sdt(signature: &[u8; 4], index: usize) -> Option<*const AcpiHeader> {
    let mut cnt = 0usize;
    let use_xsdt = USE_XSDT.load(Ordering::Relaxed);

    let rsdt_ptr = RSDT_PTR.load(Ordering::Relaxed) as *const Rsdt;
    let header_len = unsafe { (*rsdt_ptr).header.length } as usize;
    let entries = (header_len - core::mem::size_of::<AcpiHeader>()) / if use_xsdt { 8 } else { 4 };

    let ptrs_start = unsafe { (rsdt_ptr as *const u8).add(core::mem::size_of::<AcpiHeader>()) };

    for i in 0..entries {
        let phys: u64 = if use_xsdt {
            unsafe { (ptrs_start as *const u64).add(i).read_unaligned() }
        } else {
            unsafe { (ptrs_start as *const u32).add(i).read_unaligned() as u64 }
        };

        let hdr = (phys + HHDM_OFFSET.load(Ordering::Relaxed)) as *const AcpiHeader;

        let matches_sig = unsafe { (*hdr).signature == *signature };
        let checksum_ok = unsafe { checksum(hdr as *const u8, (*hdr).length as usize) == 0 };

        if matches_sig && checksum_ok {
            if cnt == index {
                return Some(hdr);
            }
            cnt += 1;
        }
    }

    None
}
