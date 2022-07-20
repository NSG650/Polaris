#include <asm/asm.h>
#include <debug/debug.h>
#include <klibc/mem.h>
#include <mm/vmm.h>
#include <sched/sched.h>
#include <sys/apic.h>
#include <sys/prcb.h>
#include <sys/timer.h>

lock_t resched_lock;
extern void resched_context_switch(registers_t *reg);
uint64_t tick = 0;

void sched_resched_now(void) {
	int nex_index =
		sched_get_next_thread(prcb_return_current_cpu()->thread_index);
	apic_eoi();
	timer_sched_oneshot(32, 20000);
	prcb_return_current_cpu()->running_thread = NULL;
	prcb_return_current_cpu()->thread_index = nex_index;
	sti();
	for (;;)
		halt();
}

uint64_t timer_sched_tick(void) {
	return tick;
}

void resched(registers_t *reg) {
	tick++;
	spinlock_acquire_or_wait(resched_lock);
	struct thread *running_thrd = prcb_return_current_cpu()->running_thread;
	if (running_thrd) {
		running_thrd->reg = *reg;
		running_thrd->stack = prcb_return_current_cpu()->user_stack;
		running_thrd->state = THREAD_READY_TO_RUN;
		spinlock_drop(running_thrd->lock);
	}
	int nex_index =
		sched_get_next_thread(prcb_return_current_cpu()->thread_index);
	spinlock_drop(resched_lock);
	if (nex_index == -1) {
		// we're idle
		apic_eoi();
		timer_sched_oneshot(32, 20000);
		prcb_return_current_cpu()->running_thread = NULL;
		prcb_return_current_cpu()->thread_index = nex_index;
		sti();
		for (;;)
			halt();
	}
	running_thrd = threads.data[nex_index];
	prcb_return_current_cpu()->running_thread = running_thrd;
	prcb_return_current_cpu()->thread_index = nex_index;
	prcb_return_current_cpu()->cpu_tss.rsp0 = running_thrd->kernel_stack;

	prcb_return_current_cpu()->user_stack = running_thrd->stack;
	prcb_return_current_cpu()->kernel_stack = running_thrd->kernel_stack;
	prcb_return_current_cpu()->running_thread->state = THREAD_NORMAL;

	apic_eoi();
	timer_sched_oneshot(32, running_thrd->runtime);

	vmm_switch_pagemap(running_thrd->mother_proc->process_pagemap);
	resched_context_switch(&running_thrd->reg);
}
