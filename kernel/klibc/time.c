#include <errno.h>
#include <klibc/time.h>
#include <klibc/vec.h>
#include <limine.h>
#include <stddef.h>
#include <stdint.h>

static volatile struct limine_boot_time_request boot_time_request = {
	.id = LIMINE_BOOT_TIME_REQUEST, .revision = 0};

struct timespec time_monotonic = {0, 0};
struct timespec time_realtime = {0, 0};

static lock_t timers_lock = 0;
static vec_t(struct timer *) armed_timers;

struct timer *timer_new(struct timespec when) {
	struct timer *timer = kmalloc(sizeof(struct timer));
	if (timer == NULL) {
		return NULL;
	}

	timer->when = when;
	timer->fired = false;
	timer->index = -1;

	timer_arm(timer);
	return timer;
}

void timer_arm(struct timer *timer) {
	spinlock_acquire_or_wait(timers_lock);

	timer->index = armed_timers.length;
	timer->fired = false;

	vec_push(&armed_timers, timer);
	spinlock_drop(timers_lock);
}

void timer_disarm(struct timer *timer) {
	spinlock_acquire_or_wait(timers_lock);

	if (armed_timers.length == 0 || timer->index == -1 ||
		timer->index >= armed_timers.length) {
		goto cleanup;
	}

	armed_timers.data[timer->index] =
		armed_timers.data[armed_timers.length - 1];
	armed_timers.data[timer->index]->index = timer->index;
	vec_splice(&armed_timers, armed_timers.length - 1, 1);

	timer->index = -1;

cleanup:
	spinlock_drop(timers_lock);
}

void time_init(void) {
	struct limine_boot_time_response *boot_time_resp =
		boot_time_request.response;

	time_realtime.tv_sec = boot_time_resp->boot_time;
	syscall_register_handler(0x13a, syscall_getclock);
}

void timer_handler(void) {
	struct timespec interval = {.tv_sec = 0,
								.tv_nsec = 1000000000 / TIMER_FREQ};

	time_monotonic = timespec_add(time_monotonic, interval);
	time_realtime = timespec_add(time_realtime, interval);

	if (spinlock_acquire(timers_lock)) {
		for (int i = 0; i < armed_timers.length; i++) {
			struct timer *timer = armed_timers.data[i];
			if (timer->fired) {
				continue;
			}

			timer->when = timespec_sub(timer->when, interval);
			if (timer->when.tv_sec == 0 && timer->when.tv_nsec == 0) {
				event_trigger(&timer->event, false);
				timer->fired = true;
			}
		}

		spinlock_drop(timers_lock);
	}
}

void syscall_getclock(struct syscall_arguments *args) {
	int which = args->args0;
	struct timespec *out = (void *)args->args1;
	int ret = -1;

	switch (which) {
		case CLOCK_REALTIME:
		case CLOCK_REALTIME_COARSE:
			*out = time_realtime;
			ret = 0;
			goto cleanup;
		case CLOCK_BOOTTIME:
		case CLOCK_MONOTONIC:
		case CLOCK_MONOTONIC_RAW:
		case CLOCK_MONOTONIC_COARSE:
			*out = time_monotonic;
			ret = 0;
			goto cleanup;
		case CLOCK_PROCESS_CPUTIME_ID:
		case CLOCK_THREAD_CPUTIME_ID:
			*out = (struct timespec){.tv_sec = 0, .tv_nsec = 0};
			ret = 0;
			goto cleanup;
	}

	errno = EINVAL;

cleanup:
	args->ret = ret;
}
