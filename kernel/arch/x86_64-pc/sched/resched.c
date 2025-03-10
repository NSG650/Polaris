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
extern struct thread *sleeping_threads;
extern struct thread *thread_list;
extern lock_t wakeup_lock;
extern lock_t electric_chair_lock;
extern lock_t thread_lock;

extern void resched_context_switch(registers_t *reg);

void sched_resched_now(void) {
	sti();
	asm volatile("int 0x30");
}

uint64_t timer_sched_tick(void) {
	return prcb_return_current_cpu()->sched_ticks;
}

void resched(registers_t *reg) {
	cli();

	vmm_switch_pagemap(kernel_pagemap);
	prcb_return_current_cpu()->sched_ticks++;
	timer_stop_sched();
	timer_handler();

	struct thread *running_thrd = prcb_return_current_cpu()->running_thread;

	if (running_thrd) {
		spinlock_drop(&running_thrd->lock);
		if (running_thrd->marked_for_execution) {
			struct process *mother_proc = running_thrd->mother_proc;
			vec_remove(&mother_proc->process_threads, running_thrd);
			if (mother_proc->process_threads.length < 1) {
				process_kill(mother_proc, false);
			}
			spinlock_acquire_or_wait(&thread_lock);
			sched_remove_thread_from_list(&thread_list, running_thrd);
			spinlock_drop(&thread_lock);
			spinlock_acquire_or_wait(&electric_chair_lock);
			sched_add_thread_to_list(&threads_on_the_death_row, running_thrd);
			spinlock_drop(&electric_chair_lock);
			running_thrd = NULL;
		} else {
			running_thrd->reg = *reg;
			running_thrd->fs_base = read_fs_base();
			running_thrd->gs_base = read_user_gs();
			prcb_return_current_cpu()->fpu_save(running_thrd->fpu_storage);

			running_thrd->stack = prcb_return_current_cpu()->user_stack;
			running_thrd->kernel_stack =
				prcb_return_current_cpu()->kernel_stack;

			if (running_thrd->state == THREAD_NORMAL)
				running_thrd->state = THREAD_READY_TO_RUN;

			running_thrd->last_scheduled = timer_count();
		}
	}

	running_thrd = sched_get_next_thread(running_thrd);

	if (running_thrd == NULL) {
		apic_eoi();
		prcb_return_current_cpu()->running_thread = NULL;
		timer_sched_oneshot(48, 20000);
		sti();
		for (;;) {
			halt();
		}
	}

	prcb_return_current_cpu()->running_thread = running_thrd;
	prcb_return_current_cpu()->cpu_tss.rsp0 = running_thrd->kernel_stack;
	prcb_return_current_cpu()->cpu_tss.ist2 = running_thrd->pf_stack;

	prcb_return_current_cpu()->user_stack = running_thrd->stack;
	prcb_return_current_cpu()->kernel_stack = running_thrd->kernel_stack;
	prcb_return_current_cpu()->running_thread->state = THREAD_NORMAL;
	prcb_return_current_cpu()->fpu_restore(running_thrd->fpu_storage);

	set_fs_base(running_thrd->fs_base);
	// set_user_gs(running_thrd->gs_base);

	vmm_switch_pagemap(running_thrd->mother_proc->process_pagemap);
	write_cr("3", read_cr("3"));

	apic_eoi();
	timer_sched_oneshot(48, running_thrd->runtime);
	sti();
	resched_context_switch(&running_thrd->reg);
}
