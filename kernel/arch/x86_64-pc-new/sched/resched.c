#include <debug/debug.h>
#include <fw/madt.h>
#include <locks/spinlock.h>
#include <sched/sched.h>
#include <sys/apic.h>
#include <sys/prcb.h>

lock_t resched_lock = {0};
extern uint32_t smp_bsp_lapic_id;

void resched(registers_t *reg) {
	spinlock_acquire_or_wait(&resched_lock);

	// pretty ugly solution
	if (lapic_get_id() == smp_bsp_lapic_id) {
		for (int i = 0; i < madt_local_apics.length; i++) {
			struct madt_lapic *lapic = madt_local_apics.data[i];
			if (lapic_get_id() == lapic->apic_id)
				continue;
			apic_send_ipi(lapic->apic_id, 0x30);
		}
	}

	(void)reg;
	apic_eoi();

	spinlock_drop(&resched_lock);
}
