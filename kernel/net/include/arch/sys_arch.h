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

struct mbox {
    lock_t lock;
    sys_sem_t free, queued;
    size_t length;
    int count, head, next;
    void **slots;
    int valid;
};
typedef struct mbox sys_mbox_t;

struct mutex {
    lock_t lock;
    struct event ev;
};
typedef struct mutex sys_mutex_t;

typedef struct thread *sys_thread_t;
typedef void (*lwip_thread_fn)(void *arg);

#ifndef SYS_ARCH_TIMEOUT
#define SYS_ARCH_TIMEOUT 0xffffffffUL
#endif

#ifndef LWIP_NETCONN_THREAD_SEM_ALLOC
err_t LWIP_NETCONN_THREAD_SEM_ALLOC();
#endif

#ifndef LWIP_NETCONN_THREAD_SEM_FREE
err_t LWIP_NETCONN_THREAD_SEM_FREE();
#endif

#ifndef LWIP_NETCONN_THREAD_SEM_GET
sys_sem_t *LWIP_NETCONN_THREAD_SEM_GET();
#endif

#endif