#include <cpu/cr.h>
#include <cpu/smp.h>
#include <cpu_features.h>
#include <cpuid.h>
#include <debug/debug.h>
#include <klibc/mem.h>
#include <klibc/vec.h>
#include <locks/spinlock.h>
#include <mm/pmm.h>
#include <mm/slab.h>
#include <mm/vmm.h>
#include <sys/apic.h>
#include <sys/gdt.h>
#include <sys/halt.h>
#include <sys/isr.h>
#include <sys/prcb.h>
#include <sys/timer.h>

size_t fpu_storage_size = 0;
void (*fpu_save)(void *ctx) = NULL;
void (*fpu_restore)(void *ctx) = NULL;

prcb_vec_t prcbs;
static uint8_t bsp_lapic_core = 0;
static size_t cpu_count = 0;
static size_t initialized_cores = 0;
bool is_smp = false;
lock_t smp_lock = 0;

char prcb_names[21][3] = {"Aa", "Ab", "E",	"H",  "Ua", "Ub", "Z",
						  "F",	"Ga", "Gb", "Gc", "Gd", "Ja", "Jb",
						  "Na", "Nb", "Ra", "Rb", "T",	"V",  "Y"};

extern void amd_syscall_entry(void);

static void smp_init_core(struct limine_smp_info *smp_info) {
	spinlock_acquire_or_wait(smp_lock);
	cli();
	gdt_init();
	isr_install();

	vmm_switch_pagemap(kernel_pagemap);

	struct prcb *ap = kmalloc(sizeof(struct prcb));

	if (cpu_count > 21) {
		ltoa(smp_info->lapic_id, ap->name, 16);
	} else {
		strcpy(ap->name, prcb_names[smp_info->lapic_id]);
	}

	ap->cpu_number = smp_info->lapic_id;
	ap->running_thread = NULL;
	ap->thread_index = 0;

	ap->cpu_tss.rsp0 = (uint64_t)pmm_allocz(STACK_SIZE / PAGE_SIZE);
	ap->cpu_tss.rsp0 += MEM_PHYS_OFFSET + STACK_SIZE;

	ap->cpu_tss.ist1 = (uint64_t)pmm_allocz(STACK_SIZE / PAGE_SIZE);
	ap->cpu_tss.ist1 += MEM_PHYS_OFFSET + STACK_SIZE;

	vec_push(&prcbs, ap);
	set_kernel_gs((uint64_t)prcbs.data[ap->cpu_number]);
	set_user_gs((uint64_t)prcbs.data[ap->cpu_number]);
	gdt_load_tss((size_t)&prcb_return_current_cpu()->cpu_tss);

	// SSE/SSE2
	uint64_t cr0 = 0;
	cr0 = read_cr("0");
	cr0 &= ~(1 << 2);
	cr0 |= (1 << 1);
	write_cr("0", cr0);

	uint64_t cr4 = 0;
	cr4 = read_cr("4");
	cr4 |= (3 << 9);
	write_cr("4", cr4);

	// Enable syscall in EFER
	wrmsr(0xc0000080, rdmsr(0xc0000080) | 1);

	// Set up syscall
	wrmsr(0xc0000081, 0x13000800000000);
	// Syscall entry address
	wrmsr(0xc0000082, (uint64_t)amd_syscall_entry);
	// Flags mask
	wrmsr(0xc0000084, (uint64_t) ~((uint32_t)0x2));

	// Security features
	uint32_t a = 0, b = 0, c = 0, d = 0;
	__get_cpuid(7, &a, &b, &c, &d);
	if (b & CPUID_SMEP) {
		cr4 = read_cr("4");
		cr4 |= (1 << 20); // Enable SMEP
		write_cr("4", cr4);
	}

	if (b & CPUID_SMAP) {
		cr4 = read_cr("4");
		cr4 |= (1 << 21); // Enable SMAP
		write_cr("4", cr4);
		asm("clac");
	}

	if (c & CPUID_UMIP) {
		cr4 = read_cr("4");
		cr4 |= (1 << 11); // Enable UMIP
		write_cr("4", cr4);
	}

	__get_cpuid(1, &a, &b, &c, &d);
	if (c & bit_XSAVE) {
		// Enable XSAVE and x{get,set}bv
		cr4 = read_cr("4");
		cr4 |= (uint64_t)1 << 18;
		write_cr("4", cr4);

		uint64_t xcr0 = 0;
		xcr0 |= (uint64_t)1 << 0;
		xcr0 |= (uint64_t)1 << 1;

		if (c & bit_AVX) {
			xcr0 |= (uint64_t)1 << 2;
		}

		__get_cpuid(7, &a, &b, &c, &d);
		if (b & bit_AVX512F) {
			xcr0 |= (uint64_t)1 << 5;
			xcr0 |= (uint64_t)1 << 6;
			xcr0 |= (uint64_t)1 << 7;
		}

		wrxcr(0, xcr0);

		__cpuid(13, a, b, c, d);

		fpu_storage_size = c;
		fpu_save = xsave;
		fpu_restore = xrstor;
	} else {
		fpu_storage_size = 512;
		fpu_save = fxsave;
		fpu_restore = fxrstor;
	}

	lapic_init(smp_info->lapic_id);

	kprintf("CPU%u: %s online!\n", prcb_return_current_cpu()->cpu_number,
			prcb_return_current_cpu()->name);

	initialized_cores++;
	spinlock_drop(smp_lock);
	if (prcb_return_current_cpu()->cpu_number != bsp_lapic_core) {
		timer_sched_oneshot(32, 20000);
		sti();
		for (;;)
			halt();
		__builtin_unreachable();
	}
}

void smp_init(struct limine_smp_response *smp_info) {
	vec_init(&prcbs);
	bsp_lapic_core = smp_info->bsp_lapic_id;
	cpu_count = smp_info->cpu_count;
	kprintf("SMP: Bringing up the AP cores\n");
	for (size_t i = 0; i < smp_info->cpu_count; i++) {
		if (smp_info->cpus[i]->lapic_id == bsp_lapic_core) {
			kprintf("SMP: BSP Core %u\n", bsp_lapic_core);
			smp_init_core((void *)smp_info->cpus[i]);
			continue;
		}
		spinlock_acquire_or_wait(smp_lock);
		smp_info->cpus[i]->goto_address = smp_init_core;
		spinlock_drop(smp_lock);
		timer_sleep(100);
	}
	while (initialized_cores != cpu_count)
		pause();
	is_smp = true;
	kprintf("SMP: %u CPUs installed in the system\n",
			prcb_return_installed_cpus());
}

size_t prcb_return_installed_cpus(void) {
	return initialized_cores;
}
