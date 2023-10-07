#include <asm/asm.h>
#include <cpu/smp.h>
#include <debug/debug.h>
#include <fw/madt.h>
#include <klibc/mem.h>
#include <klibc/time.h>
#include <mm/vmm.h>
#include <sched/sched.h>
#include <sys/apic.h>
#include <sys/prcb.h>
#include <sys/timer.h>

lock_t resched_lock = {0};

extern uint32_t smp_bsp_lapic_id;

extern void resched_context_switch(registers_t *reg);

void sched_resched_now(void) {
	sti();
	apic_send_ipi(lapic_get_id(), 48);
}

uint64_t timer_sched_tick(void) {
	return prcb_return_current_cpu()->sched_ticks;
}

void resched(registers_t *reg) {
	vmm_switch_pagemap(kernel_pagemap);
	prcb_return_current_cpu()->sched_ticks++;

	timer_stop_sched();

	for (int i = 0; i < sleeping_threads.length &&
					prcb_return_current_cpu()->cpu_number == 0;
		 i++) {
		struct thread *th = sleeping_threads.data[i];
		if (th->sleeping_till < timer_get_abs_count()) {
			th->sleeping_till = 0;
			th->state = THREAD_READY_TO_RUN;
			vec_remove(&sleeping_threads, th);
		}
	}

	//	spinlock_acquire_or_wait(&resched_lock);

	struct thread *running_thrd = prcb_return_current_cpu()->running_thread;
	if (running_thrd) {
		running_thrd->reg = *reg;
		running_thrd->stack = prcb_return_current_cpu()->user_stack;

		prcb_return_current_cpu()->fpu_save(running_thrd->fpu_storage);

		if (running_thrd->state == THREAD_NORMAL)
			running_thrd->state = THREAD_READY_TO_RUN;
		if (running_thrd->mother_proc->state == PROCESS_NORMAL)
			running_thrd->mother_proc->state = PROCESS_READY_TO_RUN;

		running_thrd->last_scheduled =
			timer_count() - running_thrd->last_scheduled;
		running_thrd->fs_base = read_fs_base();
		spinlock_drop(&running_thrd->lock);
	}

	int nex_index =
		sched_get_next_thread(prcb_return_current_cpu()->thread_index);

	//	spinlock_drop(&resched_lock);

	if (nex_index == -1) {
		// we're idle
		apic_eoi();
		timer_sched_oneshot(48, 20000);
		prcb_return_current_cpu()->running_thread = NULL;
		prcb_return_current_cpu()->thread_index = nex_index;
		sti();
		for (;;)
			halt();
	}

	running_thrd = threads.data[nex_index];

	// spinlock_acquire_or_wait(&resched_lock);

	prcb_return_current_cpu()->fpu_restore(running_thrd->fpu_storage);

	// Don't fuck with the kernel gs
	if (running_thrd->mother_proc != processes.data[0]) { // :))))
		set_fs_base(running_thrd->fs_base);
	}

	prcb_return_current_cpu()->running_thread = running_thrd;
	prcb_return_current_cpu()->thread_index = nex_index;
	prcb_return_current_cpu()->cpu_tss.rsp0 = running_thrd->kernel_stack;

	prcb_return_current_cpu()->user_stack = running_thrd->stack;
	prcb_return_current_cpu()->kernel_stack = running_thrd->kernel_stack;
	prcb_return_current_cpu()->running_thread->state = THREAD_NORMAL;
	prcb_return_current_cpu()->running_thread->mother_proc->state =
		PROCESS_NORMAL;

	apic_eoi();
	timer_sched_oneshot(48, running_thrd->runtime);
	sti();

	// spinlock_drop(&resched_lock);

	vmm_switch_pagemap(running_thrd->mother_proc->process_pagemap);
	resched_context_switch(&running_thrd->reg);
}
