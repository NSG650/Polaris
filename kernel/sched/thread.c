#include <klibc/vec.h>
#include <locks/spinlock.h>
#include <mem/liballoc.h>
#include <sched/thread.h>

lock_t thread_lock;
struct thread *threads;
uint64_t tid = 1;
uint64_t thread_count = 0;

void thread_create(uintptr_t pc_address, uint64_t arguments) {
	spinlock_acquire(thread_lock);
	struct thread thrd = {0};
	thrd.tid = tid;
	tid++;
	thrd.state = 0;
	thrd.runtime = 5000;
#if defined(__x86_64__)
	thrd.reg.rip = pc_address;
	thrd.reg.rdi = arguments;
	thrd.reg.rsp = (uint64_t)kmalloc(STACK_SIZE);
	thrd.reg.rsp += STACK_SIZE;
	thrd.reg.cs = 0x08;
	thrd.reg.ss = 0x10;
	thrd.reg.rflags = 0x202;
#endif
	vector_add(&threads, thrd);
	thread_count++;
	spinlock_drop(thread_lock);
}
