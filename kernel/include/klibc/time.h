#ifndef TIME_H
#define TIME_H

#include <klibc/event.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#define TIMER_FREQ 1000

#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1
#define CLOCK_PROCESS_CPUTIME_ID 2
#define CLOCK_THREAD_CPUTIME_ID 3
#define CLOCK_MONOTONIC_RAW 4
#define CLOCK_REALTIME_COARSE 5
#define CLOCK_MONOTONIC_COARSE 6
#define CLOCK_BOOTTIME 7

struct timer {
	int index;
	bool fired;
	struct timespec when;
	struct event event;
};

extern struct timespec time_monotonic;
extern struct timespec time_realtime;

static inline struct timespec timespec_add(struct timespec a,
										   struct timespec b) {
	if (a.tv_nsec + b.tv_nsec > 999999999) {
		a.tv_nsec = (a.tv_nsec + b.tv_nsec) - 1000000000;
		a.tv_sec++;
	} else {
		a.tv_nsec += b.tv_nsec;
	}
	a.tv_sec += b.tv_sec;
	return a;
}

static inline struct timespec timespec_sub(struct timespec a,
										   struct timespec b) {
	if (b.tv_nsec > a.tv_nsec) {
		a.tv_nsec = 999999999 - (b.tv_nsec - a.tv_nsec);
		if (a.tv_sec == 0) {
			a.tv_sec = a.tv_nsec = 0;
			return a;
		}
		a.tv_sec--;
	} else {
		a.tv_nsec -= b.tv_nsec;
	}

	if (b.tv_sec > a.tv_sec) {
		a.tv_sec = a.tv_nsec = 0;
		return a;
	}
	a.tv_sec -= b.tv_sec;

	return a;
}

struct timer *timer_new(struct timespec when);
void timer_arm(struct timer *timer);
void timer_disarm(struct timer *timer);
void timer_handler(uint64_t ns);
void time_nsleep(uint64_t ns);

void time_init(void);
void syscall_getclock(struct syscall_arguments *args);
void syscall_nanosleep(struct syscall_arguments *args);

#endif
