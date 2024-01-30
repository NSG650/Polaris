#include <debug/debug.h>
#include <errno.h>
#include <kernel.h>
#include <mm/slab.h>
#include <sched/futex.h>
#include <sched/sched.h>
#include <sys/prcb.h>
#include <sys/timer.h>

#define VIRTUAL_STACK_ADDR 0x70000000000
#define MMAP_ANON_BASE 0x80000000000

lock_t sched_lock = {0};
bool sched_runit = false;

struct thread *thread_list = NULL;
struct process *process_list = NULL;
struct thread *sleeping_threads = NULL;
dead_process_vec_t dead_processes = {0};

int64_t tid = 0;
int64_t pid = 0;

lock_t thread_lock = {0};
lock_t process_lock = {0};

struct resource *std_console_device = NULL;

struct thread *sched_get_next_thread(struct thread *thrd) {
	struct thread *this = NULL;
	if (thrd) {
		this = thrd->next;
	} else {
		this = thread_list;
	}

	while (this) {
		if (this->state != THREAD_READY_TO_RUN) {
			this = this->next;
			continue;
		}
		if (spinlock_acquire(&this->lock))
			return this;
		this = this->next;
	}

	return NULL;
}

static struct thread *sched_tid_to_thread(int64_t t) {
	struct thread *this = thread_list;
	while (this) {
		if (this->tid == t)
			return this;
		this = this->next;
	}
	return NULL;
}

static struct process *sched_pid_to_process(int64_t p) {
	struct process *this = process_list;
	while (this) {
		if (this->pid == p)
			return this;
		this = this->next;
	}
	return NULL;
}

static struct dead_process *
sched_return_recently_dead_child_process(struct process *parent_process) {
	for (int i = dead_processes.length - 1; i >= 0; i--) {
		if (dead_processes.data[i]->parent_process == parent_process)
			return dead_processes.data[i];
	}
	return NULL;
}

static struct dead_process *sched_pid_to_dead_child_process(int64_t p) {
	for (int i = 0; i < dead_processes.length; i++) {
		if (dead_processes.data[i]->pid == p)
			return dead_processes.data[i];
	}
	return NULL;
}

void sched_add_thread_to_list(struct thread **thrd_list, struct thread *thrd) {
	struct thread *this = *thrd_list;

	if (this == NULL) {
		*thrd_list = thrd;
		return;
	}

	while (this->next) {
		this = this->next;
	}
	this->next = thrd;
}

void sched_remove_thread_from_list(struct thread **thrd_list,
								   struct thread *thrd) {
	struct thread *this = *thrd_list;
	struct thread *next = NULL;

	if (this == thrd) {
		*thrd_list = this->next;
		return;
	}

	while (this) {
		next = this->next;
		if (next == thrd) {
			this->next = next->next;
			next->next = NULL;
			return;
		}
		this = next;
	}
}

void sched_add_process_to_list(struct process **proc_list,
							   struct process *proc) {
	struct process *this = *proc_list;

	if (this == NULL) {
		*proc_list = proc;
		return;
	}

	while (this->next) {
		this = this->next;
	}
	this->next = proc;
}

void sched_remove_process_from_list(struct process **proc_list,
									struct process *proc) {
	struct process *this = *proc_list;
	struct process *next = NULL;

	if (this == proc) {
		*proc_list = this->next;
		return;
	}

	while (this) {
		next = this->next;
		if (next == proc) {
			this->next = next->next;
			next->next = NULL;
			return;
		}
		this = next;
	}
}

void syscall_kill(struct syscall_arguments *args) {
	struct process *proc = sched_pid_to_process((int64_t)args->args0);
	args->ret = 0;
	if (!proc)
		args->ret = -1;
	else
		process_kill(proc, false);
}

void syscall_exit(struct syscall_arguments *args) {
	(void)args;
	prcb_return_current_cpu()->running_thread->mother_proc->waitee.exit_code =
		(uint8_t)args->args0;
	process_kill(prcb_return_current_cpu()->running_thread->mother_proc, false);
}

void syscall_getpid(struct syscall_arguments *args) {
	args->ret = prcb_return_current_cpu()->running_thread->mother_proc->pid;
}

void syscall_getppid(struct syscall_arguments *args) {
	args->ret = -1;
	if (prcb_return_current_cpu()
			->running_thread->mother_proc->parent_process) {
		args->ret = prcb_return_current_cpu()
						->running_thread->mother_proc->parent_process->pid;
	}
}

void syscall_nanosleep(struct syscall_arguments *args) {
	struct __kernel_timespec *from_user =
		(struct __kernel_timespec *)args->args0;
	uint64_t seconds_to_ns = from_user->tv_sec * 1000000000;
	uint64_t total_sleep = seconds_to_ns + from_user->tv_nsec;

	thread_sleep(prcb_return_current_cpu()->running_thread, total_sleep);

	args->ret = 0;
}

void syscall_fork(struct syscall_arguments *args) {
	struct thread *running_thread = prcb_return_current_cpu()->running_thread;
	struct process *running_process = running_thread->mother_proc;
	args->ret = process_fork(running_process, running_thread);
}

void syscall_execve(struct syscall_arguments *args) {
	if (!process_execve((char *)args->args0, (char **)args->args1,
						(char **)args->args2)) {
		args->ret = -1;
	}
}

void syscall_uname(struct syscall_arguments *args) {
	struct utsname {
		char sysname[65];
		char nodename[65];
		char release[65];
		char version[65];
		char machine[65];
		char domainname[65];
	};

	struct utsname *from_user = (struct utsname *)args->args0;

	strncpy(from_user->sysname, "Polaris", sizeof(from_user->sysname));
	strncpy(from_user->nodename, "localhost", sizeof(from_user->nodename));
	strncpy(from_user->release, "0.0.0", sizeof(from_user->release));

#ifndef GIT_VERSION
	strncpy(from_user->version, "unknown", sizeof(from_user->version));
#else
	strncpy(from_user->version, GIT_VERSION, sizeof(from_user->version));
#endif

#if defined(__x86_64__)
	strncpy(from_user->machine, "x86_64", sizeof(from_user->machine));
#else
	strncpy(from_user->machine, "unknown", sizeof(from_user->machine));
#endif

	args->ret = 0;
}

void syscall_waitpid(struct syscall_arguments *args) {
#define WNOHANG 1
	int pid_to_wait_on = (int)args->args0;
	int *status = (int *)args->args1;
	int mode = (int)args->args2;

	struct process *waiter_proc =
		prcb_return_current_cpu()->running_thread->mother_proc;

	if (!waiter_proc->child_processes.length) {
		errno = ECHILD;
		args->ret = -1;
		return;
	}

	if (pid_to_wait_on < -1 || pid_to_wait_on == 0) {
		errno = EINVAL;
		args->ret = -1;
		return;
	}

	struct process *waitee_proc = NULL;
	struct event **events = NULL;
	size_t event_count = 0;

	if (pid_to_wait_on == -1) {
		events = kmalloc(sizeof(struct event *) *
						 waiter_proc->child_processes.length);
		event_count = waiter_proc->child_processes.length;
		for (int i = 0; i < waiter_proc->child_processes.length; i++) {
			events[i] = &waiter_proc->child_processes.data[i]->death_event;
		}
	}

	else {
		events = kmalloc(sizeof(struct event *));
		for (int i = 0; i < waiter_proc->child_processes.length; i++) {
			if (waiter_proc->child_processes.data[i]->pid == pid_to_wait_on) {
				waitee_proc = waiter_proc->child_processes.data[i]->pid;
				break;
			}
		}
		if (waitee_proc == NULL) {
			errno = ECHILD;
			args->ret = -1;
			return;
		}
		event_count = 1;
		events[0] = &waitee_proc->death_event;
	}

	bool block = (mode & WNOHANG) == 0;
	ssize_t which = event_await(events, event_count, block);

	if (which == -1) {
		if (block) {
			args->ret = 0;
			return;
		} else {
			errno = EINTR;
			args->ret = -1;
			return;
		}
	}

	struct dead_process *dead_proc = NULL;
	if (pid_to_wait_on == -1) {
		dead_proc = sched_return_recently_dead_child_process(waiter_proc);
	} else {
		dead_proc = sched_pid_to_dead_child_process(pid_to_wait_on);
	}

	*status = dead_proc->exit_code;
	args->ret = dead_proc->pid;
}

void sched_init(uint64_t args) {
	vec_init(&dead_processes);

	syscall_register_handler(0x27, syscall_getpid);
	syscall_register_handler(0x67, syscall_puts);
	syscall_register_handler(0x6e, syscall_getppid);
	syscall_register_handler(0x3c, syscall_exit);
	syscall_register_handler(0x3e, syscall_kill);
	syscall_register_handler(0x9d, syscall_prctl);
	syscall_register_handler(0x23, syscall_nanosleep);
	syscall_register_handler(0x39, syscall_fork);
	syscall_register_handler(0x3b, syscall_execve);
	syscall_register_handler(0x3f, syscall_uname);
	syscall_register_handler(0x72, syscall_waitpid);

	futex_init();

	process_create("kernel_tasks", PROCESS_READY_TO_RUN, 100000,
				   (uintptr_t)kernel_main, args, false, NULL);
	sched_runit = true;
}

void process_create(char *name, uint8_t state, uint64_t runtime,
					uintptr_t pc_address, uint64_t arguments, bool user,
					struct process *parent_process) {
	spinlock_acquire_or_wait(&process_lock);

	struct process *proc = kmalloc(sizeof(struct process));
	memzero(proc, sizeof(struct process));

	strncpy(proc->name, name, 256);

	proc->runtime = runtime;
	proc->state = state;
	proc->pid = pid++;

	process_setup_context(proc, user);

	proc->cwd = vfs_root;
	proc->stack_top = VIRTUAL_STACK_ADDR;

	if (parent_process) {
		proc->parent_process = parent_process;
		if (proc->parent_process->cwd) {
			proc->cwd = parent_process->cwd;
		}
		proc->umask = parent_process->umask;
		proc->mmap_anon_base = parent_process->mmap_anon_base;
		vec_push(&parent_process->child_processes, proc);
	} else {
		proc->umask = S_IWGRP | S_IWOTH;
		proc->mmap_anon_base = MMAP_ANON_BASE;
	}
	proc->next = NULL;

	vec_init(&proc->process_threads);
	vec_init(&proc->child_processes);

	sched_add_process_to_list(&process_list, proc);
	thread_create(pc_address, arguments, user, proc);

	spinlock_drop(&process_lock);
}

bool process_create_elf(char *name, uint8_t state, uint64_t runtime, char *path,
						struct process *parent_process) {
	spinlock_acquire_or_wait(&process_lock);

	struct process *proc = kmalloc(sizeof(struct process));
	memzero(proc, sizeof(struct process));

	strncpy(proc->name, name, 256);

	proc->runtime = runtime;
	proc->state = state;
	proc->pid = pid++;

	process_setup_context(proc, true);

	proc->cwd = vfs_root;
	proc->stack_top = VIRTUAL_STACK_ADDR;

	if (parent_process) {
		proc->parent_process = parent_process;
		if (proc->parent_process->cwd) {
			proc->cwd = parent_process->cwd;
		}
		proc->umask = parent_process->umask;
		proc->mmap_anon_base = parent_process->mmap_anon_base;
		vec_push(&parent_process->child_processes, proc);
	} else {
		proc->umask = S_IWGRP | S_IWOTH;
		proc->mmap_anon_base = MMAP_ANON_BASE;
	}

	struct auxval auxv, ld_aux;
	struct vfs_node *node = vfs_get_node(vfs_root, path, true);
	const char *ld_path = NULL;

	if (!node ||
		!elf_load(proc->process_pagemap, node->resource, 0, &auxv, &ld_path)) {
		return false;
	}

	// HACK: ld_path for processes that don't depend on ld.so points to
	// kmalloced memory which is not null checking the first letter is '/' or
	// not to know if a program needs ld

	uint64_t entry = auxv.at_entry;

	if (ld_path && ld_path[0] == '/') {
		struct vfs_node *ld_node = vfs_get_node(vfs_root, ld_path, true);

		if (!ld_node || !elf_load(proc->process_pagemap, ld_node->resource,
								  0x40000000, &ld_aux, NULL)) {
			return false;
		}
		entry = ld_aux.at_entry;
	}

	proc->auxv = auxv;

	for (int i = 0; i < 3; i++)
		fdnum_create_from_resource(proc, std_console_device, 0, i, true);

	proc->next = NULL;

	vec_init(&proc->process_threads);
	vec_init(&proc->child_processes);

	sched_add_process_to_list(&process_list, proc);
	thread_create(entry, 0, true, proc);

	spinlock_drop(&process_lock);

	return true;
}

int64_t process_fork(struct process *proc, struct thread *thrd) {
	spinlock_acquire_or_wait(&process_lock);

	struct process *fproc = kmalloc(sizeof(struct process));
	memzero(fproc, sizeof(struct process));
	strncpy(fproc->name, proc->name, 256);

	process_fork_context(proc, fproc);

	fproc->mmap_anon_base = proc->mmap_anon_base;
	fproc->cwd = proc->cwd;
	fproc->umask = proc->umask;
	fproc->pid = pid++;
	fproc->parent_process = proc;
	fproc->next = NULL;

	vec_init(&fproc->child_processes);
	vec_init(&fproc->process_threads);

	vec_push(&proc->child_processes, fproc);

	for (int i = 0; i < MAX_FDS; i++) {
		if (proc->fds[i] == NULL) {
			continue;
		}

		if (fdnum_dup(proc, i, fproc, i, 0, true, false) != i) {
			kfree(fproc);
			break;
		}
	}

	sched_add_process_to_list(&process_list, fproc);
	thread_fork(thrd, fproc);

	fproc->state = PROCESS_READY_TO_RUN;
	spinlock_drop(&process_lock);

	return fproc->pid;
}

bool process_execve(char *path, char **argv, char **envp) {
	spinlock_acquire(&process_lock);

	struct thread *thread = prcb_return_current_cpu()->running_thread;
	struct process *proc = thread->mother_proc;

	struct auxval auxv, ld_aux;
	struct vfs_node *node = vfs_get_node(proc->cwd, path, true);
	const char *ld_path;

	if (!node) {
		spinlock_drop(&process_lock);
		return false;
	}

	struct pagemap *old_pagemap = proc->process_pagemap;
	process_setup_context(proc, true);

	if (!elf_load(proc->process_pagemap, node->resource, 0, &auxv, &ld_path)) {
		proc->process_pagemap = old_pagemap;
		spinlock_drop(&process_lock);
		errno = ENOENT;
		return false;
	}

	// HACK: ld_path for processes that don't depend on ld.so points to
	// kmalloced memory which is not null checking the first letter is '/' or
	// not to know if a program needs ld

	uint64_t entry = auxv.at_entry;

	if (ld_path && ld_path[0] == '/') {
		struct vfs_node *ld_node = vfs_get_node(vfs_root, ld_path, true);

		if (!ld_node || !elf_load(proc->process_pagemap, ld_node->resource,
								  0x40000000, &ld_aux, NULL)) {
			proc->process_pagemap = old_pagemap;
			spinlock_drop(&process_lock);
			errno = ENOENT;
			return false;
		}
		entry = ld_aux.at_entry;
	}

	strncpy(proc->name, path, 256);

	for (int i = 0; i < proc->process_threads.length; i++) {
		if (proc->process_threads.data[i] != thread) {
			thread_kill(proc->process_threads.data[i], 0);
		}
	}

	proc->mmap_anon_base = MMAP_ANON_BASE;
	proc->stack_top = VIRTUAL_STACK_ADDR;
	proc->state = PROCESS_READY_TO_RUN;

	// We no longer exist. There is no point in saving anything now.
	prcb_return_current_cpu()->running_thread = NULL;

	proc->auxv = auxv;

	spinlock_drop(&process_lock);

	thread_execve(proc, thread, entry, argv, envp);

	vmm_switch_pagemap(kernel_pagemap);

	sched_resched_now();
	return false;
}

void process_kill(struct process *proc, bool crash) {
	spinlock_acquire_or_wait(&process_lock);
	cli();

	if (proc->pid < 2) {
		panic("Attempted to kill init!\n");
	}

	struct dead_process *dead_proc = kmalloc(sizeof(struct dead_process));
	dead_proc->parent_process = proc->parent_process;
	dead_proc->pid = proc->pid;

	bool are_we_killing_ourselves = false;
	if (prcb_return_current_cpu()->running_thread->mother_proc == proc) {
		are_we_killing_ourselves = true;
	}

	for (int i = 0; i < proc->process_threads.length; i++) {
		thread_kill(proc->process_threads.data[i], false);
	}

	if (proc->parent_process) {
		vec_remove(&proc->parent_process->child_processes, proc);
	}

	struct process *init_proc = process_list->next;

	for (int i = 0; i < proc->child_processes.length; i++) {
		struct process *child_proc = proc->child_processes.data[i];
		child_proc->parent_process = init_proc;
		vec_push(&init_proc->child_processes, child_proc);
	}

	if (are_we_killing_ourselves && !crash) {
		dead_proc->exit_code = proc->waitee.exit_code;
		dead_proc->was_it_killed = false;

	} else {
		dead_proc->exit_code = -1;
		dead_proc->was_it_killed = true;
	}

	process_destroy_context(proc);

	vec_push(&dead_processes, dead_proc);

	vec_deinit(&proc->child_processes);
	sched_remove_process_from_list(&process_list, proc);

	event_trigger(&proc->death_event, false);

	kfree(proc);

	sti();
	spinlock_drop(&process_lock);

	if (are_we_killing_ourselves) {
		prcb_return_current_cpu()->running_thread = NULL;
		sched_resched_now();
	}
}

void thread_create(uintptr_t pc_address, uint64_t arguments, bool user,
				   struct process *proc) {
	spinlock_acquire_or_wait(&thread_lock);

	struct thread *thrd = kmalloc(sizeof(struct thread));
	memzero(thrd, sizeof(struct thread));
	thrd->tid = tid++;
	thrd->runtime = proc->runtime;
	thrd->mother_proc = proc;

	thread_setup_context(thrd, pc_address, arguments, user);

	thrd->next = NULL;
	spinlock_init(thrd->lock);
	thrd->state = THREAD_READY_TO_RUN;

	vec_push(&proc->process_threads, thrd);
	sched_add_thread_to_list(&thread_list, thrd);

	spinlock_drop(&thread_lock);
}

void thread_fork(struct thread *pthrd, struct process *fproc) {
	spinlock_acquire_or_wait(&thread_lock);
	struct thread *thrd = kmalloc(sizeof(struct thread));
	memzero(thrd, sizeof(struct thread));

	thrd->tid = tid++;
	thrd->state = THREAD_READY_TO_RUN;
	thrd->runtime = pthrd->runtime;
	thrd->mother_proc = fproc;
	spinlock_init(thrd->lock);

	thread_fork_context(pthrd, thrd);

	thrd->last_scheduled = 0;
	thrd->sleeping_till = 0;
	sched_add_thread_to_list(&thread_list, thrd);
	vec_push(&fproc->process_threads, thrd);

	spinlock_drop(&thread_lock);
}

void thread_execve(struct process *proc, struct thread *thrd,
				   uintptr_t pc_address, char **argv, char **envp) {
	spinlock_acquire_or_wait(&thread_lock);

	memzero(thrd, sizeof(struct thread));

	thrd->tid = tid++;
	thrd->state = THREAD_READY_TO_RUN;
	thrd->runtime = proc->runtime;
	spinlock_init(thrd->lock);
	thrd->mother_proc = proc;

	thread_setup_context_for_execve(thrd, pc_address, argv, envp);

	spinlock_drop(&thread_lock);
}

void thread_sleep(struct thread *thrd, uint64_t ns) {
	spinlock_acquire_or_wait(&thread_lock);

	thrd->state = THREAD_SLEEPING;
	thrd->sleeping_till = timer_get_sleep_ns(ns);

	sched_remove_thread_from_list(&thread_list, thrd);
	sched_add_thread_to_list(&sleeping_threads, thrd);

	spinlock_drop(&thread_lock);
	sched_resched_now();
}

void thread_kill(struct thread *thrd, bool reschedule) {
	spinlock_acquire_or_wait(&thread_lock);

	vec_remove(&thrd->mother_proc->process_threads, thrd);
	thread_destroy_context(thrd);
	sched_remove_thread_from_list(&thread_list, thrd);
	kfree(thrd);

	spinlock_drop(&thread_lock);

	if (reschedule) {
		sched_resched_now();
	}
}
