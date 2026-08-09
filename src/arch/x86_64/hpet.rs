use super::acpi;
use super::acpi::AcpiHeader;
use crate::log;
use crate::mm::virt::HHDM_OFFSET;
use core::sync::atomic::{AtomicU64, Ordering};

#[repr(C, packed)]
struct HpetSdt {
    header: AcpiHeader,
    event_timer_block_id: u32,
    address_space_id: u8,
    register_bit_width: u8,
    register_bit_offset: u8,
    reserved: u8,
    address: u64,
    hpet_number: u8,
    minimum_tick: u16,
    page_protection: u8,
}

const REG_GENERAL_CAPABILITIES: u64 = 0x00;
const REG_GENERAL_CONFIG: u64 = 0x10;
const REG_MAIN_COUNTER: u64 = 0xF0;

const GENERAL_CONFIG_ENABLE: u64 = 1 << 0;

static HPET_BASE: AtomicU64 = AtomicU64::new(0);
static HPET_PERIOD_FS: AtomicU64 = AtomicU64::new(0);

fn read_reg(reg: u64) -> u64 {
    let addr = HPET_BASE.load(Ordering::Relaxed) + reg;
    unsafe { core::ptr::read_volatile(addr as *const u64) }
}

fn write_reg(reg: u64, value: u64) {
    let addr = HPET_BASE.load(Ordering::Relaxed) + reg;
    unsafe { core::ptr::write_volatile(addr as *mut u64, value) };
}

pub fn init() {
    let hdr = acpi::find_sdt(b"HPET", 0).expect("Did not find APIC table?") as u64;

    let hpet = hdr as *const HpetSdt;
    let phys_base = unsafe { (*hpet).address };

    HPET_BASE.store(
        phys_base + HHDM_OFFSET.load(Ordering::Relaxed),
        Ordering::Relaxed,
    );

    let caps = read_reg(REG_GENERAL_CAPABILITIES);
    let period_fs = caps >> 32;
    HPET_PERIOD_FS.store(period_fs, Ordering::Relaxed);

    log!("Using HPET at {:#x}\r\n", HPET_BASE.load(Ordering::Relaxed));

    write_reg(REG_MAIN_COUNTER, 0);
    write_reg(REG_GENERAL_CONFIG, GENERAL_CONFIG_ENABLE);
}

pub fn read_counter() -> u64 {
    read_reg(REG_MAIN_COUNTER)
}

pub fn now_ns() -> usize {
    let ticks = read_counter();
    let period_fs = HPET_PERIOD_FS.load(Ordering::Relaxed);
    ((ticks as u128 * period_fs as u128) / 1_000_000) as usize
}

pub fn stall(us: usize) {
    let target = now_ns() + us * 1000;
    while now_ns() < target {
        core::hint::spin_loop();
    }
}

pub fn sleep(ms: usize) {
    stall(ms * 1000);
}
