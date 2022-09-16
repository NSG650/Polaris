#include <errno.h>
#include <sched/futex.h>
#include <sched/sched.h>

futex_entry_t futexes;
extern lock_t sched_lock;

bool futex_wait(uint32_t value, uint32_t *futex, struct thread *thrd) {
	if (value != *futex)
		return false;
	struct futex_entry *f = kmalloc(sizeof(struct futex_entry));
	f->address = futex;
	f->thrd = thrd;
	f->value = value;
	spinlock_acquire_or_wait(sched_lock);
	thrd->state = THREAD_WAITING_FOR_FUTEX;
	spinlock_drop(sched_lock);

	// resched NOW!
	sched_resched_now();

	// won't make it here anyways
	return true;
}

bool futex_wake(uint32_t *futex) {
	struct futex_entry *match = NULL;
	for (int i = 0; i < futexes.length; i++) {
		match = futexes.data[i];
		if (match->address == futex) {
			spinlock_acquire_or_wait(sched_lock);
			match->thrd->state = THREAD_READY_TO_RUN;
			spinlock_drop(sched_lock);
			vec_remove(&futexes, match);
			return true;
		}
	}
	return false;
}

void syscall_futex(struct syscall_arguments *args) {
	uint32_t *raw_user_addr = (uint32_t *)args->args0;
	int opcode = args->args1;
	uint32_t value = args->args2;
	uint32_t *from_kernel_address =
		(uint32_t *)syscall_helper_user_to_kernel_address(
			(uintptr_t)raw_user_addr);

	struct thread *thrd = prcb_return_current_cpu()->running_thread;

	if (!from_kernel_address) {
		errno = -EFAULT;
		return;
	}

	switch (opcode) {
		case FUTEX_WAIT:
		case FUTEX_WAIT_BITSET:
			if (!futex_wait(value, from_kernel_address, thrd)) {
				errno = -EFAULT;
				return;
			}
			return;
		case FUTEX_WAKE:
		case FUTEX_WAKE_BITSET:
			futex_wake(from_kernel_address);
			return;
		default:
			errno = -EINVAL;
			return;
	}
}

void futex_init(void) {
	vec_init(&futexes);
	syscall_register_handler(0xca, syscall_futex);
}
