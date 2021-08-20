#include "cpu.h"
#include "../sys/hpet.h"
#include "../kernel/panic.h"
#include "../klibc/asm.h"
#include "../klibc/printf.h"

uint64_t cpu_tsc_frequency;

#define MAX_TSC_CALIBRATIONS 4

size_t cpu_fpu_storage_size;

void (*cpu_fpu_save)(void *);
void (*cpu_fpu_restore)(void *);


static inline void invlpg(void *addr) {
    __asm__ volatile (
        "invlpg %0"
        :
        : "m" (FLAT_PTR(addr))
        : "memory"
    );
}


bool cpuid(uint32_t leaf, uint32_t subleaf,
                         uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    uint32_t cpuid_max;
    __asm__ volatile ("cpuid"
                  : "=a" (cpuid_max)
                  : "a" (leaf & 0x80000000) : "rbx", "rcx", "rdx");
    if (leaf > cpuid_max)
        return false;
    __asm__ volatile ("cpuid"
                  : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
                  : "a" (leaf), "c" (subleaf));
    return true;
}

 uint64_t rdtsc() {
    uint32_t edx, eax;
    __asm__ volatile ("rdtsc"
                  : "=a" (eax), "=d" (edx));
    return ((uint64_t)edx << 32) | eax;
}

#define IA32_TSC_DEADLINE 0x6e0

 uint64_t rdmsr(uint32_t msr) {
    uint32_t edx, eax;
    __asm__ volatile ("rdmsr"
                  : "=a" (eax), "=d" (edx)
                  : "c" (msr)
                  : "memory");
    return ((uint64_t)edx << 32) | eax;
}

static void wrmsr(uint32_t msr, uint64_t value) {
    uint32_t edx = value >> 32;
    uint32_t eax = (uint32_t)value;
    __asm__ volatile ("wrmsr"
                  :
                  : "a" (eax), "d" (edx), "c" (msr)
                  : "memory");
}

static void wrxcr(uint32_t i, uint64_t value) {
    uint32_t edx = value >> 32;
    uint32_t eax = (uint32_t)value;
    __asm__ volatile ("xsetbv"
                  :
                  : "a" (eax), "d" (edx), "c" (i)
                  : "memory");
}

static void xsave(void *region) {
    __asm__ volatile ("xsave %0"
                  : "+m" (FLAT_PTR(region))
                  : "a" (0xffffffff), "d" (0xffffffff)
                  : "memory");
}

static void xrstor(void *region) {
    __asm__ volatile ("xrstor %0"
                  :
                  : "m" (FLAT_PTR(region)), "a" (0xffffffff), "d" (0xffffffff)
                  : "memory");
}

static void fxsave(void *region) {
    __asm__ volatile ("fxsave %0"
                  : "+m" (FLAT_PTR(region))
                  :
                  : "memory");
}

static void fxrstor(void *region) {
    __asm__ volatile ("fxrstor %0"
                  :
                  : "m" (FLAT_PTR(region))
                  : "memory");
}

void cpu_init() {
	// First enable SSE/SSE2 as it is baseline for x86_64
    uint64_t cr0 = 0;
    cr0 = read_cr("0");
    cr0 &= ~(1 << 2);
    cr0 |=  (1 << 1);
    write_cr("0", cr0);

    uint64_t cr4 = 0;
    cr4 = read_cr("4");
    cr4 |= (3 << 9);
    write_cr("4", cr4);

    // Initialise the PAT
    uint64_t pat_msr = rdmsr(0x277);
    pat_msr &= 0xffffffff;
    // write-protect / write-combining
    pat_msr |= (uint64_t)0x0105 << 32;
    wrmsr(0x277, pat_msr);

    uint32_t a, b, c, d;
    cpuid(1, 0, &a, &b, &c, &d);
	if ((c & CPUID_XSAVE)) {
        cr4 = read_cr("4");
        cr4 |= (1 << 18); // Enable XSAVE and x{get, set}bv
        write_cr("4", cr4);

        uint64_t xcr0 = 0;
        xcr0 |= (1 << 0); // Save x87 state with xsave
        xcr0 |= (1 << 1); // Save SSE state with xsave

        if ((c & CPUID_AVX))
            xcr0 |= (1 << 2); // Enable AVX and save AVX state with xsave

        if (cpuid(7, 0, &a, &b, &c, &d)) {
            if ((b & CPUID_AVX512)) {
                xcr0 |= (1 << 5); // Enable AVX-512
                xcr0 |= (1 << 6); // Enable management of ZMM{0 -> 15}
                xcr0 |= (1 << 7); // Enable management of ZMM{16 -> 31}
            }
        }
        wrxcr(0, xcr0);

        cpu_fpu_storage_size = (size_t)c;

        cpu_fpu_save = xsave;
        cpu_fpu_restore = xrstor;
    }
    else {
        cpu_fpu_storage_size = 512; // Legacy size for fxsave
        cpu_fpu_save = fxsave;
        cpu_fpu_restore = fxrstor;
    }
    cpuid(0x1, 0, &a, &b, &c, &d);
    if (!(c & CPUID_TSC_DEADLINE)) {
        PANIC("CPU: No TSC-deadline mode!!!");
        for (;;);
    }
    // Check for invariant TSC
    cpuid(0x80000007, 0, &a, &b, &c, &d);
    if (!(d & CPUID_INVARIANT_TSC)) {
        PANIC("CPU: No invariant TSC!!!");
        for (;;);
    }

    // Calibrate the TSC
    for (int i = 0; i < MAX_TSC_CALIBRATIONS; i++) {
        uint64_t initial_tsc_reading = rdtsc();

        // Wait 1 millisecond
        hpet_usleep(1000);

        uint64_t final_tsc_reading = rdtsc();

        uint64_t freq = (final_tsc_reading - initial_tsc_reading) * 1000;
        printf("CPU: TSC reading #%d yielded a frequency of %u Hz.\n", i, freq);

        cpu_tsc_frequency += freq;
    }

	// Average out all readings
    cpu_tsc_frequency /= MAX_TSC_CALIBRATIONS;
    printf("CPU: TSC frequency fixed at %u Hz.\n", cpu_tsc_frequency);
}

