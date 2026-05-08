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
};
typedef struct sem sys_sem_t;

#define SYS_ARCH_MBOX_SIZE 256

struct mbox {
    lock_t lock;
    sys_sem_t free, queued;
    int count, head, next;
    void *slots[SYS_ARCH_MBOX_SIZE];
    int valid;
};
typedef struct mbox sys_mbox_t;

typedef struct thread *sys_thread_t;
typedef lock_t sys_mutex_t;
typedef void (*lwip_thread_fn)(void *arg);

#ifndef SYS_ARCH_TIMEOUT
#define SYS_ARCH_TIMEOUT 0xffffffffUL
#endif

#endif