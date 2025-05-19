#include <debug/debug.h>
#include <errno.h>
#include <sched/futex.h>
#include <sched/sched.h>
#include <sys/prcb.h>

static lock_t futex_lock = {0};
static HASHMAP_TYPE(struct futex_entry *) futex_hashmap = HASHMAP_INIT(256);

int futex_wait(uint32_t value, uint32_t *futex, struct thread *thrd) {
	if (*futex != value) {
		errno = EAGAIN;
		return -1;
	}

	struct futex_entry *entry = NULL;

	if (HASHMAP_GET(&futex_hashmap, entry, &futex, sizeof(uint32_t *))) {
		goto wait;
	}

	entry = kmalloc(sizeof(struct futex_entry));
	struct event *futex_event = kmalloc(sizeof(struct event));
	memzero(futex_event, sizeof(struct event));

	entry->value = value;
	entry->address = futex;
	entry->thrd = thrd;
	entry->event = futex_event;

	spinlock_acquire_or_wait(&futex_lock);
	HASHMAP_INSERT(&futex_hashmap, &futex, sizeof(uint32_t *), entry);
	spinlock_drop(&futex_lock);

wait:
	if (event_await(&entry->event, 1, true) < 0) {
		errno = EINTR;
		return -1;
	}
	return 0;
}

int futex_wake(uint32_t *futex) {
	spinlock_acquire_or_wait(&futex_lock);
	struct futex_entry *entry = NULL;
	if (HASHMAP_GET(&futex_hashmap, entry, &futex, sizeof(uint32_t *))) {
		event_trigger(entry->event, false);
	}
	spinlock_drop(&futex_lock);
	return 0;
}

void syscall_futex(struct syscall_arguments *args) {
	uint32_t *raw_user_addr = (uint32_t *)args->args0;
	int opcode = args->args1;
	uint32_t value = args->args2;
	*(volatile uint32_t *)raw_user_addr; // Ensure the page isn't demand paged
	uint32_t *raw_kernel_addr =
		(uint32_t *)syscall_helper_user_to_kernel_address(
			(uintptr_t)raw_user_addr);
	struct thread *thrd = sched_get_running_thread();

	if (!raw_kernel_addr) {
		errno = EFAULT;
		args->ret = -1;
		return;
	}

	switch (opcode) {
		case FUTEX_WAIT:
		case FUTEX_WAIT_BITSET:
			args->ret = futex_wait(value, raw_kernel_addr, thrd);
			break;
		case FUTEX_WAKE:
		case FUTEX_WAKE_BITSET:
			args->ret = futex_wake(raw_kernel_addr);
			break;
		default:
			args->ret = -1;
			errno = EINVAL;
			break;
	}
}

void futex_init(void) {
	uintptr_t phys = 0;
	HASHMAP_INSERT(&futex_hashmap, &phys, sizeof(uintptr_t), NULL);
	syscall_register_handler(0xca, syscall_futex);
}
