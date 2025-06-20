#include <asm/asm.h>
#include <cpu/cr.h>
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

extern uint32_t smp_bsp_lapic_id;

extern void resched_context_switch(registers_t *reg);

void sched_yield(bool save) {
	cli();
	timer_stop_sched();

	struct thread *thrd = sched_get_running_thread();

	if (save) {
		spinlock_acquire_or_wait(&thrd->yield_lock);
	} else {
		prcb_return_current_cpu()->running_thread = NULL;
	}

	apic_send_ipi(prcb_return_current_cpu()->lapic_id, 48);
	sti();

	if (save) {
		spinlock_acquire_or_wait(
			&thrd->yield_lock); // the lock is released when the thread is
								// rescheduled
		spinlock_drop(&thrd->yield_lock);
	} else {
		for (;;) {
			halt();
		}
	}
}

void resched(registers_t *reg) {
	cli();

	vmm_switch_pagemap(kernel_pagemap);
	prcb_return_current_cpu()->sched_ticks++;
	timer_stop_sched();

	struct thread *running_thrd = prcb_return_current_cpu()->running_thread;
	uint64_t runtime = running_thrd == NULL ? 20000 : running_thrd->runtime;

	if (prcb_return_current_cpu()->lapic_id == smp_bsp_lapic_id) {
		timer_handler(runtime * 1000);
	}

	if (running_thrd) {
		spinlock_drop(&running_thrd->yield_lock);
		running_thrd->reg = *reg;
		running_thrd->fs_base = read_fs_base();
		running_thrd->gs_base = read_user_gs();
		prcb_return_current_cpu()->fpu_save(running_thrd->fpu_storage);
		if (running_thrd->state == THREAD_NORMAL) {
			running_thrd->state = THREAD_READY_TO_RUN;
		}
		running_thrd->last_scheduled = timer_count();
		running_thrd->stack = prcb_return_current_cpu()->user_stack;
		spinlock_drop(&running_thrd->lock);
	}

	running_thrd = sched_get_next_thread(running_thrd);

	if (running_thrd == NULL) {
		apic_eoi();
		prcb_return_current_cpu()->running_thread = NULL;
		timer_sched_oneshot(48, 20000);
		for (;;) {
			sti();
			halt();
		}
	}

	prcb_return_current_cpu()->running_thread = running_thrd;
	prcb_return_current_cpu()->cpu_tss.ist2 = running_thrd->pf_stack;

	prcb_return_current_cpu()->kernel_stack = running_thrd->kernel_stack;
	prcb_return_current_cpu()->user_stack = running_thrd->stack;
	prcb_return_current_cpu()->running_thread->state = THREAD_NORMAL;
	prcb_return_current_cpu()->fpu_restore(running_thrd->fpu_storage);

	set_fs_base(running_thrd->fs_base);
	// set_user_gs(running_thrd->gs_base);

	vmm_switch_pagemap(running_thrd->mother_proc->process_pagemap);

	apic_eoi();
	timer_sched_oneshot(48, running_thrd->runtime);
	resched_context_switch(&running_thrd->reg);
}
