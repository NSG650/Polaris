#include <debug/debug.h>
#include <fw/madt.h>
#include <locks/spinlock.h>
#include <sched/sched.h>
#include <sys/apic.h>
#include <sys/prcb.h>
#include <mm/vmm.h>

lock_t resched_lock = {0};

extern uint32_t smp_bsp_lapic_id;

extern void resched_context_switch(registers_t *reg);

void sched_resched_now(void) {
	sti();
	asm volatile("int 0x30");
}

uint64_t timer_sched_tick(void) {
	return prcb_return_current_cpu()->sched_ticks;
}

void resched(registers_t *reg) {
	spinlock_acquire_or_wait(&resched_lock);

	vmm_switch_pagemap(kernel_pagemap);
	prcb_return_current_cpu()->sched_ticks++;

	(void)reg;

	apic_eoi();

	spinlock_drop(&resched_lock);
}
