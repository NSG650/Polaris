#include <debug/debug.h>
#include <klibc/event.h>
#include <locks/spinlock.h>
#include <sched/sched.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/prcb.h>
#include <types.h>

extern lock_t thread_lock;

static ssize_t check_for_pending(struct event **events, size_t num_events) {
	for (size_t i = 0; i < num_events; i++) {
		if (events[i]->pending > 0) {
			events[i]->pending--;
			return i;
		}
	}
	return -1;
}

static void attach_listeners(struct event **events, size_t num_events,
							 struct thread *thread) {
	thread->attached_events_i = 0;

	for (size_t i = 0; i < num_events; i++) {
		struct event *event = events[i];

		if (event->listeners_i == EVENT_MAX_LISTENERS) {
			panic("Event listeners exhausted\n");
		}

		struct event_listener *listener =
			&event->listeners[event->listeners_i++];
		listener->thread = thread;
		listener->which = i;

		if (thread->attached_events_i == MAX_EVENTS) {
			panic("Listening on too many events\n");
		}

		thread->attached_events[thread->attached_events_i++] = event;
	}
}

static void detach_listeners(struct thread *thread) {
	for (size_t i = 0; i < thread->attached_events_i; i++) {
		struct event *event = thread->attached_events[i];

		for (size_t j = 0; j < event->listeners_i; j++) {
			struct event_listener *listener = &event->listeners[j];
			if (listener->thread != thread) {
				continue;
			}

			event->listeners[j] = event->listeners[--event->listeners_i];
			break;
		}
	}

	thread->attached_events_i = 0;
}

static void lock_events(struct event **events, size_t num_events) {
	for (size_t i = 0; i < num_events; i++) {
		spinlock_acquire_or_wait(&events[i]->lock);
	}
}

static void unlock_events(struct event **events, size_t num_events) {
	for (size_t i = 0; i < num_events; i++) {
		spinlock_drop(&events[i]->lock);
	}
}

ssize_t event_await(struct event **events, size_t num_events, bool block) {
	cli();
	ssize_t ret = -1;

	struct thread *thread = sched_get_running_thread();

	lock_events(events, num_events);

	ssize_t i = check_for_pending(events, num_events);
	if (i != -1) {
		ret = i;
		unlock_events(events, num_events);
		goto cleanup;
	}

	if (!block) {
		unlock_events(events, num_events);
		goto cleanup;
	}

	attach_listeners(events, num_events, thread);
	unlock_events(events, num_events);

	thread->state = THREAD_WAITING_FOR_EVENT;
	sched_yield(true);

	cli();
	ret = thread->which_event;

	lock_events(events, num_events);
	detach_listeners(thread);
	unlock_events(events, num_events);

cleanup:
	sti();
	return ret;
}

size_t event_trigger(struct event *event, bool drop) {
	if (!event) {
		return 0;
	}

#if defined(__x86_64__)
	uint64_t flags = 0;
	asm volatile("pushfq; pop %0" : "=rm"(flags));
	bool old_state = flags & (1 << 9);
#endif

	cli();
	spinlock_acquire_or_wait(&event->lock);

	size_t ret = 0;
	if (event->listeners_i == 0) {
		if (!drop) {
			event->pending++;
		}
		ret = 0;
		goto cleanup;
	}
	for (size_t i = 0; i < event->listeners_i; i++) {
		struct event_listener *listener = &event->listeners[i];
		spinlock_acquire_or_wait(&thread_lock);
		struct thread *thread = listener->thread;
		thread->which_event = listener->which;
		thread->state = THREAD_READY_TO_RUN;
		spinlock_drop(&thread_lock);
	}
	ret = event->listeners_i;
	event->listeners_i = 0;

cleanup:
	spinlock_drop(&event->lock);
	if (old_state) {
		sti();
	} else {
		cli();
	}
	return ret;
}
