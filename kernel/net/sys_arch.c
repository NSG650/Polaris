#include "arch/sys_arch.h"
#include <debug/debug.h>
#include <klibc/time.h>
#include <mm/slab.h>
#include <sys/timer.h>

err_t sys_mutex_new(sys_mutex_t *mutex) {
	mutex->lock = 0;
	mutex->last_owner = NULL;
	return ERR_OK;
}

void sys_mutex_lock(sys_mutex_t *mutex) {
	spinlock_acquire_or_wait((lock_t *)mutex);
}

void sys_mutex_unlock(sys_mutex_t *mutex) {
	spinlock_drop((lock_t *)mutex);
}

void sys_mutex_free(sys_mutex_t *mutex) {
	mutex->lock = 0;
	mutex->last_owner = NULL;
}

int sys_mutex_valid(sys_mutex_t *mutex) {
	return mutex != NULL;
}

void sys_mutex_set_invalid(sys_mutex_t *mutex) {
	(void)mutex;
}

err_t sys_sem_new(sys_sem_t *sem, u8_t count) {
	sys_mutex_new((sys_mutex_t *)&sem->lock);
	sem->valid = 1;
	sem->val = count;
	return ERR_OK;
}

void sys_sem_signal(sys_sem_t *sem) {
	spinlock_acquire_or_wait(&sem->lock);
	event_trigger(&sem->ev, false);
	spinlock_drop(&sem->lock);
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout) {
	spinlock_acquire_or_wait(&sem->lock);
	struct timer *timer = NULL;
	size_t event_count = 1;
	struct event *events[2] = {&sem->ev, NULL};

	if (timeout) {
		size_t ns = timeout * 1000000;
		struct timespec duration = {.tv_sec = ns / 1000000000,
									.tv_nsec = ns % 1000000000};

		timer = timer_new(duration);
		if (timer == NULL) {
			spinlock_drop(&sem->lock);
			return SYS_ARCH_TIMEOUT;
		}

		events[1] = &timer->event;
		event_count = 2;
	}
	spinlock_drop(&sem->lock);

	int ret = event_await(events, event_count, true);

	if (timeout) {
		timer_disarm(timer);
		kfree(timer);
	}

	return ret != 0 ? SYS_ARCH_TIMEOUT : 0;
}

void sys_sem_free(sys_sem_t *sem) {
	sys_sem_new(sem, 0);
}

int sys_sem_valid(sys_sem_t *sem) {
	spinlock_acquire_or_wait(&sem->lock);
	int ret = sem->valid;
	spinlock_drop(&sem->lock);
	return ret;
}

void sys_sem_set_invalid(sys_sem_t *sem) {
	spinlock_acquire_or_wait(&sem->lock);
	sem->valid = 0;
	spinlock_drop(&sem->lock);
}

err_t sys_mbox_new(sys_mbox_t *mbox, int size) {
	if (size == 0) {
		size = 512;
	}
	size *= 2; // I am having trust issues
	memzero(mbox, sizeof(sys_mbox_t));
	sys_mutex_new((sys_mutex_t *)&mbox->lock);
	mbox->valid = 1;
	sys_sem_new(&mbox->free, size);
	sys_sem_new(&mbox->queued, 0);
	mbox->head = -1;
	mbox->next = 0;
	mbox->length = size;
	mbox->slots = kmalloc(sizeof(void *) * size);
	return ERR_OK;
}

void sys_mbox_post(sys_mbox_t *mbox, void *msg) {
	sys_mutex_lock((sys_mutex_t *)&mbox->lock);
	while (mbox->count == mbox->length) {
		sys_mutex_unlock((sys_mutex_t *)&mbox->lock);
		sys_arch_sem_wait(&mbox->free, 0);
		sys_mutex_lock((sys_mutex_t *)&mbox->lock);
	}

	int slot = mbox->next;
	mbox->next = (slot + 1) % mbox->length;
	mbox->slots[slot] = msg;
	mbox->count++;
	if (mbox->head == -1)
		mbox->head = slot;

	sys_sem_signal(&mbox->queued);
	sys_mutex_unlock((sys_mutex_t *)&mbox->lock);
}

err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg) {
	sys_mutex_lock((sys_mutex_t *)&mbox->lock);
	if (mbox->count == mbox->length) {
		sys_mutex_unlock((sys_mutex_t *)&mbox->lock);
		return ERR_MEM;
	}

	int slot = mbox->next;
	mbox->next = (slot + 1) % mbox->length;
	mbox->slots[slot] = msg;
	mbox->count++;
	if (mbox->head == -1)
		mbox->head = slot;

	sys_sem_signal(&mbox->queued);
	sys_mutex_unlock((sys_mutex_t *)&mbox->lock);

	return ERR_OK;
}

err_t sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg) {
	return sys_mbox_trypost(mbox, msg);
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout) {
	sys_mutex_lock(&mbox->lock);
	if (mbox->head == -1) {
		sys_mutex_unlock(&mbox->lock);
		u32_t waited = sys_arch_sem_wait(&mbox->queued, timeout);
		if (waited == SYS_ARCH_TIMEOUT) {
			return waited;
		}
		sys_mutex_lock(&mbox->lock);
	}

	int slot = mbox->head;

	if (msg)
		*msg = mbox->slots[slot];

	mbox->head = (slot + 1) % mbox->length;
	mbox->count--;
	if (mbox->count == 0)
		mbox->head = -1;

	sys_sem_signal(&mbox->free);
	sys_mutex_unlock(&mbox->lock);
	return 0;
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg) {
	sys_mutex_lock(&mbox->lock);
	int slot = mbox->head;
	if (slot == -1) {
		sys_mutex_unlock(&mbox->lock);
		return SYS_ARCH_TIMEOUT;
	}

	if (msg)
		*msg = mbox->slots[slot];

	mbox->head = (slot + 1) % mbox->length;
	mbox->count--;
	if (mbox->count == 0)
		mbox->head = -1;

	sys_sem_signal(&mbox->free);
	sys_mutex_unlock(&mbox->lock);
	return 0;
}

void sys_mbox_free(sys_mbox_t *mbox) {
	sys_mutex_lock((sys_mutex_t *)&mbox->lock);
	sys_sem_free(&mbox->free);
	sys_sem_free(&mbox->queued);
	mbox->valid = 0;
	mbox->length = 0;
	kfree(mbox->slots);
	sys_mutex_unlock((sys_mutex_t *)&mbox->lock);
}

int sys_mbox_valid(sys_mbox_t *mbox) {
	return mbox->valid;
}

void sys_mbox_set_invalid(sys_mbox_t *mbox) {
	mbox->valid = 0;
}

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg,
							int stacksize, int prio) {
	(void)name;
	thread_create((uintptr_t)thread, (uint64_t)arg, false, kernel_proc);
	return (sys_thread_t)thread;
}

u32_t sys_now(void) {
	return (u32_t)timer_count();
}

void sys_init(void) {
	kprintf("LwIP: Hello there!\n");
}
