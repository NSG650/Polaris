#include <asm/asm.h>
#include <debug/debug.h>
#include <kernel.h>
#include <klibc/vec.h>
#include <sched/sched.h>
#include <sched/thread.h>
#include <sys/timer.h>

lock_t sched_lock;
bool sched_runit = false;

int64_t sched_get_next_thread(int64_t index) {
	if (index == -1) {
		index = 0;
	} else {
		index++;
	}

	for (size_t i = 0; i < thread_count + 1; i++) {
		if ((size_t)index >= thread_count) {
			index = 0;
		}
		struct thread *thread = &threads[index];
		if (spinlock_acquire(thread->lock))
			return index;
		index++;
	}

	return -1;
}

void sched_init(void) {
	kprintf("SCHED: Creating kernel thread\n");
	threads = vector_create();
	thread_create((uintptr_t)kernel_thread, 0xdead);
}
