#ifndef LWIP_ARCH_SYS_ARCH_H
#define LWIP_ARCH_SYS_ARCH_H

#include <stdint.h>
#include <sched/sched.h>
#include <locks/spinlock.h>
#include <klibc/event.h>
#include "lwip/err.h"

struct sem {
    lock_t lock;
    int val;
    struct event ev;
    int valid;
    int name;
};
typedef struct sem *sys_sem_t;

struct mbox {
    lock_t lock;
    sys_sem_t free, queued;
    int count, head, next;
    void *slots[128];
    int valid;
};
typedef struct mbox *sys_mbox_t;

typedef struct thread *sys_thread_t;
typedef lock_t *sys_mutex_t;
typedef void (*lwip_thread_fn)(void *arg);

#endif