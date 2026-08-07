use core::arch::asm;
use core::mem::size_of;

#[repr(C, packed)]
struct GdtDesc {
    limit: u16,
    base_low: u16,
    base_mid: u8,
    access: u8,
    granularity: u8,
    base_hi: u8,
}

#[repr(C, packed)]
struct TssDesc {
    length: u16,
    base_low: u16,
    base_mid: u8,
    flags1: u8,
    flags2: u8,
    base_hi: u8,
    base_upper32: u32,
    reserved: u32,
}

#[repr(C, packed)]
struct GdtPtr {
    limit: u16,
    base: u64,
}

#[repr(C, packed)]
struct Gdt {
    entries: [GdtDesc; 5],
    tss: TssDesc,
}

#[repr(C, packed)]
pub struct Tss {
    reserved0: u32,
    pub rsp: [u64; 3],
    reserved1: u64,
    pub ist: [u64; 7],
    reserved2: u64,
    reserved3: u16,
    pub iomap_base: u16,
}

impl Tss {
    fn new() -> Self {
        Tss {
            reserved0: 0,
            rsp: [0; 3],
            reserved1: 0,
            ist: [0; 7],
            reserved2: 0,
            reserved3: 0,
            iomap_base: size_of::<Tss>() as u16,
        }
    }
}

pub const SEL_KERNEL_CODE: u16 = 0x08;
pub const SEL_KERNEL_DATA: u16 = 0x10;
pub const SEL_USER_DATA: u16 = 0x18 | 3;
pub const SEL_USER_CODE: u16 = 0x20 | 3;
pub const SEL_TSS: u16 = 0x28 | 3;

pub struct CpuGdt {
    gdt: Gdt,
    gdt_ptr: GdtPtr,
    pub tss: Tss,
}

impl CpuGdt {
    pub fn new() -> Self {
        let mut gdt = Gdt {
            entries: [
                GdtDesc {
                    limit: 0,
                    base_low: 0,
                    base_mid: 0,
                    access: 0,
                    granularity: 0,
                    base_hi: 0,
                },
                GdtDesc {
                    limit: 0,
                    base_low: 0,
                    base_mid: 0,
                    access: 0b10011010,
                    granularity: 0b00100000,
                    base_hi: 0,
                },
                GdtDesc {
                    limit: 0,
                    base_low: 0,
                    base_mid: 0,
                    access: 0b10010010,
                    granularity: 0,
                    base_hi: 0,
                },
                GdtDesc {
                    limit: 0,
                    base_low: 0,
                    base_mid: 0,
                    access: 0b11110010,
                    granularity: 0,
                    base_hi: 0,
                },
                GdtDesc {
                    limit: 0,
                    base_low: 0,
                    base_mid: 0,
                    access: 0b11111010,
                    granularity: 0b00100000,
                    base_hi: 0,
                },
            ],
            tss: TssDesc {
                length: size_of::<Tss>() as u16,
                base_low: 0,
                base_mid: 0,
                flags1: 0b10001001,
                flags2: 0,
                base_hi: 0,
                base_upper32: 0,
                reserved: 0,
            },
        };
        gdt.tss.length = size_of::<Tss>() as u16;

        CpuGdt {
            gdt,
            gdt_ptr: GdtPtr { limit: 0, base: 0 },
            tss: Tss::new(),
        }
    }

    pub fn load(&mut self) {
        let tss_addr = &self.tss as *const _ as u64;
        self.gdt.tss.base_low = tss_addr as u16;
        self.gdt.tss.base_mid = (tss_addr >> 16) as u8;
        self.gdt.tss.base_hi = (tss_addr >> 24) as u8;
        self.gdt.tss.base_upper32 = (tss_addr >> 32) as u32;

        self.gdt_ptr.limit = (size_of::<Gdt>() - 1) as u16;
        self.gdt_ptr.base = &self.gdt as *const _ as u64;

        unsafe {
            lgdt(&self.gdt_ptr);
            reload_code_segment();
            reload_data_segments(SEL_KERNEL_DATA);
            ltr(SEL_TSS);
        }
    }
}

#[inline(always)]
unsafe fn lgdt(ptr: &GdtPtr) {
    asm!("lgdt [{0}]", in(reg) ptr, options(readonly, nostack, preserves_flags));
}

#[inline(always)]
unsafe fn ltr(sel: u16) {
    asm!("ltr {0:x}", in(reg) sel, options(nostack, preserves_flags));
}

#[inline(never)]
unsafe fn reload_code_segment() {
    asm!(
        "push {sel}",
        "lea {tmp}, [rip + 2f]",
        "push {tmp}",
        "retfq",
        "2:",
        sel = const SEL_KERNEL_CODE as u64,
        tmp = lateout(reg) _,
        options(nostack)
    );
}

#[inline(always)]
unsafe fn reload_data_segments(sel: u16) {
    asm!(
        "mov ds, {0:x}",
        "mov es, {0:x}",
        "mov fs, {0:x}",
        "mov gs, {0:x}",
        "mov ss, {0:x}",
        in(reg) sel,
        options(nostack, preserves_flags)
    );
}
