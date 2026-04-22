#include <debug/debug.h>
#include <sys/timer.h>
#include "arch/sys_arch.h"
#include <mm/slab.h>

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
    event_trigger(&sem->ev, false);
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout) {
    struct event *events[] = {&sem->ev};
	if (event_await(events, 1, true) < 0) {
		return SYS_ARCH_TIMEOUT;
	}
    return 0;
}

void sys_sem_free(sys_sem_t *sem) {
    sys_sem_new(sem, 0);
}

int sys_sem_valid(sys_sem_t *sem) {
    return sem->valid;
}

void sys_sem_set_invalid(sys_sem_t *sem) {
    sem->valid = 0;
}

err_t sys_mbox_new(sys_mbox_t *mbox, int size) {
    sys_mutex_new((sys_mutex_t *)&mbox->lock);
    mbox->valid = 1;
    sys_sem_new(&mbox->free, 128);
    sys_sem_new(&mbox->queued, 0);
    mbox->head = -1;
    mbox->next = 0;
    return ERR_OK;
}

void sys_mbox_post(sys_mbox_t *mbox, void *msg) {
    sys_arch_sem_wait(&mbox->free, 0);
    sys_mutex_lock((sys_mutex_t *)&mbox->lock);
    if (mbox->count == 128) {
        sys_mutex_unlock((sys_mutex_t *)&mbox->lock);
        return;
    }

    int slot = mbox->next;
    mbox->next = (slot + 1) % 128;
    mbox->slots[slot] = msg;
    mbox->count++;
    if (mbox->head == -1)
        mbox->head = slot;

    sys_sem_signal(&mbox->queued);
    sys_mutex_unlock((sys_mutex_t *)&mbox->lock);
}

err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg) {
    sys_mutex_lock((sys_mutex_t *)&mbox->lock);
    if (mbox->count == 128) {
        sys_mutex_unlock((sys_mutex_t *)&mbox->lock);
        return ERR_MEM;
    }

    int slot = mbox->next;
    mbox->next = (slot + 1) % 128;
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
    u32_t waited = sys_arch_sem_wait(&mbox->queued, timeout);
    sys_mutex_lock(&mbox->lock);
    if (waited == SYS_ARCH_TIMEOUT) {
        sys_mutex_unlock(&mbox->lock);
        return waited;
    }

    int slot = mbox->head;
    if (slot == -1) {
        sys_mutex_unlock(&mbox->lock);
        return SYS_ARCH_TIMEOUT;
    }

    if (msg)
        *msg = mbox->slots[slot];

    mbox->head = (slot + 1) % 128;
    mbox->count--;
    if (mbox->count == 0)
        mbox->head = -1;

    sys_sem_signal(&mbox->free);
    sys_mutex_unlock(&mbox->lock);
    return waited;
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

    mbox->head = (slot + 1) % 128;
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
    sys_mutex_unlock((sys_mutex_t *)&mbox->lock);
}

int sys_mbox_valid(sys_mbox_t *mbox) {
    return mbox->valid;
}

void sys_mbox_set_invalid(sys_mbox_t *mbox) {
    mbox->valid = 0;
}

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio) {
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
