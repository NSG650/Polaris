#ifndef LWIP_LWIPOPTS_CC_H
#define LWIP_LWIPOPTS_CC_H

#include <types.h>
#include <errno.h>
#include <mm/slab.h>

#define MEM_LIBC_MALLOC 0
#define LWIP_NO_CTYPE_H 1
#define LWIP_NO_UNISTD_H 1
#define LWIP_NO_LIMITS_H 1
#define LWIP_TIMEVAL_PRIVATE 1
#define CUSTOM_IOVEC 1
#define LWIP_SO_RCVBUF 1
#define LWIP_NETCONN_FULLDUPLEX 0
#define LWIP_NETCONN_SEM_PER_THREAD 0
#define LWIP_SOCKET_SELECT 1
#define LWIP_SOCKET_POLL 1
#define LWIP_NETIF_LOOPBACK 1

#define TCPIP_MBOX_SIZE 32

#define LWIP_RAW 1
#define LWIP_DHCP 1
#define LWIP_DNS 1
#define LWIP_DEBUG 0

#define MEM_CUSTOM_ALLOCATOR            1
#define MEM_CUSTOM_FREE                 kfree
#define MEM_CUSTOM_MALLOC               kmalloc
#define MEM_CUSTOM_CALLOC               kcalloc

// raise connection limits
#define MEMP_NUM_NETCONN 100
#define MEMP_NUM_TCP_PCB 100

#define LWIP_SOCKET_OFFSET MEMP_NUM_NETCONN

// raise the server buffer (forced to do second)
#define TCP_SND_BUF 8192
#define MEMP_NUM_TCP_SEG (2 * TCP_SND_QUEUELEN)

// optimizations
#define TCP_WND (16 * TCP_MSS)
#define LWIP_CHKSUM_ALGORITHM 3

#define SYS_LIGHTWEIGHT_PROT 0
#define LWIP_COMPAT_SOCKETS 0

#endif