#include <cpu/cr.h>
#include <cpu/msr.h>
#include <cpu/smp.h>
#include <cpu_features.h>
#include <cpuid.h>
#include <debug/debug.h>
#include <klibc/mem.h>
#include <klibc/vec.h>
#include <locks/spinlock.h>
#include <mem/liballoc.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <sys/apic.h>
#include <sys/gdt.h>
#include <sys/halt.h>
#include <sys/isr.h>
#include <sys/prcb.h>
#include <sys/timer.h>

prcb_vec_t prcbs;
uint8_t bsp_lapic_core;
uint64_t cpu_count = 0;
uint64_t initialised_cores = 0;
bool is_smp = false;
lock_t smp_lock;

char prcb_names[21][3] = {"Aa", "Ab", "E",	"H",  "Ua", "Ub", "Z",
						  "F",	"Ga", "Gb", "Gc", "Gd", "Ja", "Jb",
						  "Na", "Nb", "Ra", "Rb", "T",	"V",  "Y"};

static void smp_set_gs(uint64_t address) {
	wrmsr(0xc0000101, address);
}
static uint64_t smp_read_gs(void) {
	return rdmsr(0xc0000101);
}

static void smp_init_core(struct stivale2_smp_info *smp_info) {
	spinlock_acquire_or_wait(smp_lock);
	cli();
	gdt_init();
	isr_install();
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

	// Security features

	uint32_t a = 0, b = 0, c = 0, d = 0;
	if (__get_cpuid(7, &a, &b, &c, &d)) {
		if ((b & CPUID_SMEP)) {
			cr4 = read_cr("4");
			cr4 |= (1 << 20); // Enable SMEP
			write_cr("4", cr4);
		}
	}

	if (__get_cpuid(7, &a, &b, &c, &d)) {
		if ((b & CPUID_SMAP)) {
			cr4 = read_cr("4");
			cr4 |= (1 << 21); // Enable SMAP
			write_cr("4", cr4);
			asm("clac");
		}
	}

	if (__get_cpuid(7, &a, &b, &c, &d)) {
		if ((c & CPUID_UMIP)) {
			cr4 = read_cr("4");
			cr4 |= (1 << 11); // Enable UMIP
			write_cr("4", cr4);
		}
	}

	vmm_switch_pagemap(&kernel_pagemap);
	struct prcb *ap = kmalloc(sizeof(struct prcb));
	if (cpu_count > 21) {
		ltoa(smp_info->lapic_id, ap->name, 16);
	}
	strcpy(ap->name, prcb_names[smp_info->lapic_id]);
	ap->cpu_number = smp_info->lapic_id;
	ap->running_thread = NULL;
	ap->thread_index = 0;
	ap->cpu_tss.rsp0 = smp_info->target_stack;
	ap->cpu_tss.ist1 = (uint64_t)pmm_allocz(32768 / PAGE_SIZE);
	ap->cpu_tss.ist1 += MEM_PHYS_OFFSET + 32768;
	vec_push(&prcbs, ap);
	smp_set_gs((uint64_t)prcbs.data[ap->cpu_number]);
	gdt_load_tss((size_t)&prcb_return_current_cpu()->cpu_tss);
	kprintf("CPU%d: %s online!\n", prcb_return_current_cpu()->cpu_number,
			prcb_return_current_cpu()->name);
	initialised_cores++;
	spinlock_drop(smp_lock);
	if (prcb_return_current_cpu()->cpu_number != bsp_lapic_core) {
		lapic_init(smp_info->lapic_id);
		// timer_sched_oneshot(32, 20000);
		sti();
		for (;;)
			halt();
	}
}

void smp_init(struct stivale2_struct_tag_smp *smp_info) {
	vec_init(&prcbs);
	bsp_lapic_core = smp_info->bsp_lapic_id;
	cpu_count = smp_info->cpu_count;
	kprintf("SMP: Bringing up the AP cores\n");
	for (size_t i = 0; i < smp_info->cpu_count; i++) {
		if (smp_info->smp_info[i].lapic_id == bsp_lapic_core) {
			kprintf("SMP: BSP Core %u\n", bsp_lapic_core);
			smp_init_core((void *)&smp_info->smp_info[i]);
			continue;
		}
		spinlock_acquire_or_wait(smp_lock);
		smp_info->smp_info[i].target_stack =
			(uint64_t)pmm_allocz(32768 / PAGE_SIZE);
		smp_info->smp_info[i].target_stack += MEM_PHYS_OFFSET + 32768;
		smp_info->smp_info[i].goto_address = (uint64_t)smp_init_core;
		spinlock_drop(smp_lock);
		timer_sleep(100);
	}
	while (initialised_cores != cpu_count)
		;
	is_smp = true;
	kprintf("SMP: %d CPUs installed in the system\n",
			prcb_return_installed_cpus());
}

struct prcb *prcb_return_current_cpu(void) {
	return (struct prcb *)smp_read_gs();
}

uint64_t prcb_return_installed_cpus(void) {
	return initialised_cores;
}
