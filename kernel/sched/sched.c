#include <asm/asm.h>
#include <cpu/smp.h>
#include <debug/debug.h>
#include <errno.h>
#include <fb/fb.h>
#include <kernel.h>
#include <klibc/elf.h>
#include <klibc/misc.h>
#include <klibc/vec.h>
#include <mm/mmap.h>
#include <sched/sched.h>
#include <sched/syscall.h>
#include <sys/elf.h>
#include <sys/prcb.h>
#include <sys/timer.h>

#define VIRTUAL_STACK_ADDR 0x70000000000

struct resource *std_console_device;

lock_t sched_lock = 0;
bool sched_runit = false;

thread_vec_t threads;
thread_vec_t sleeping_threads;
process_vec_t processes;
dead_process_vec_t dead_processes;

int64_t pid = 0;
lock_t process_lock = 0;

lock_t thread_lock = 0;
int64_t tid = 0;

struct process *sched_pid_to_process(int64_t process_pid) {
	for (int i = 0; i < processes.length; i++) {
		if (processes.data[i]->pid == process_pid) {
			return processes.data[i];
		}
	}
	return NULL;
}

int sched_get_next_thread(int index) {
	if (index == -1 || index > threads.length) {
		index = 0;
	} else {
		index++;
	}

	for (int i = 0; i < threads.length; i++) {
		if (index >= threads.length) {
			index = 0;
		}
		struct thread *thread = threads.data[index];

		if (thread->state != THREAD_READY_TO_RUN ||
			thread->mother_proc->state != PROCESS_READY_TO_RUN) {
			index++;
			continue;
		}

		if (spinlock_acquire(thread->lock)) {
			return index;
		}

		index++;
	}

	return -1;
}

void syscall_kill(struct syscall_arguments *args) {
	struct process *proc = sched_pid_to_process(args->args0);
	args->ret = 0;
	if (!proc)
		args->ret = -1;
	else
		process_kill(proc, 0);
}

void syscall_exit(struct syscall_arguments *args) {
	(void)args;
	prcb_return_current_cpu()->running_thread->mother_proc->waitee.exit_code =
		(uint8_t)args->args0;
	process_kill(prcb_return_current_cpu()->running_thread->mother_proc, 0);
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
						(char **)args->args2))
		args->ret = -1;
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
	int64_t pid_to_wait_on = (int64_t)args->args0;

	int *status = (int *)args->args1;
	int mode = (int)args->args2;

	struct process *waiter_process =
		prcb_return_current_cpu()->running_thread->mother_proc;

	if (!waiter_process->child_processes.length) {
		errno = ECHILD;
		args->ret = -1;
		return;
	}

	if (pid_to_wait_on < -1 || pid_to_wait_on == 0) {
		errno = EINVAL;
		args->ret = -1;
	}

	struct process *waitee_process = sched_pid_to_process(pid_to_wait_on);

	if (mode & WNOHANG) {
		args->ret = 0;
		if (pid_to_wait_on == -1) {
			if (waiter_process->waitee.parent_process == waiter_process) {
				args->ret = waiter_process->waitee.pid;
				*status = waiter_process->waitee.exit_code;
			} else {
				// time to loop over every dead process and see if our child
				// died
				for (int i = 0; i < dead_processes.length; i++) {
					if (dead_processes.data[i]->parent_process ==
						waiter_process) {
						args->ret = dead_processes.data[i]->pid;
						*status = dead_processes.data[i]->exit_code;
						break;
					}
				}
			}
		} else {
			if (waitee_process == NULL) {
				args->ret = pid_to_wait_on;
				for (int i = 0; i < dead_processes.length; i++) {
					if (dead_processes.data[i]->pid == pid_to_wait_on) {
						*status = dead_processes.data[i]->exit_code;
						break;
					}
				}
			}
		}
		return;
	}

	if (pid_to_wait_on == -1) {
		process_wait_on_processes(waiter_process,
								  waiter_process->child_processes);
		args->ret = waiter_process->waitee.pid;
		*status = waiter_process->waitee.exit_code;
		return;
	}

	if (waitee_process == NULL) {
		errno = ECHILD;
		args->ret = -1;
		return;
	}

	process_wait_on_another_process(waiter_process, waitee_process);
	args->ret = waiter_process->waitee.pid;
	*status = waiter_process->waitee.exit_code;
}

void sched_init(uint64_t args) {
	kprintf("SCHED: Creating kernel thread\n");
	vec_init(&threads);
	vec_init(&processes);
	vec_init(&sleeping_threads);
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
	process_create("kernel_tasks", 0, 2000, (uintptr_t)kernel_main, args, 0,
				   NULL);
}

void process_create(char *name, uint8_t state, uint64_t runtime,
					uintptr_t pc_address, uint64_t arguments, bool user,
					struct process *parent_process) {
	spinlock_acquire_or_wait(process_lock);
	struct process *proc = kmalloc(sizeof(struct process));
	strncpy(proc->name, name, 256);
	proc->runtime = runtime;
	proc->state = state;
	proc->pid = pid++;
	proc->state = PROCESS_READY_TO_RUN;
	memset(&proc->auxv, 0, sizeof(struct auxval));
#if defined(__x86_64__)
	if (user) {
		proc->process_pagemap = vmm_new_pagemap();
	} else {
		proc->process_pagemap = kernel_pagemap;
	}
#endif
	vec_init(&proc->process_threads);
	vec_init(&proc->child_processes);
	vec_init(&proc->waiter_processes);
	vec_push(&processes, proc);

	if (parent_process) {
		vec_push(&parent_process->child_processes, proc);
		if (parent_process->cwd != NULL) {
			proc->cwd = parent_process->cwd;
		} else {
			proc->cwd = vfs_root;
		}
		proc->umask = parent_process->umask;
		proc->mmap_anon_base = parent_process->mmap_anon_base;
		proc->process_pagemap =
			vmm_fork_pagemap(parent_process->process_pagemap);
	} else {
		proc->cwd = vfs_root;
		proc->umask = S_IWGRP | S_IWOTH;
		proc->mmap_anon_base = 0x80000000000;
	}
	thread_create(pc_address, arguments, user, proc);
	spinlock_drop(process_lock);
}

bool process_create_elf(char *name, uint8_t state, uint64_t runtime, char *path,
						struct process *parent_process) {
	spinlock_acquire_or_wait(process_lock);
	struct process *proc = kmalloc(sizeof(struct process));
	strncpy(proc->name, name, 256);
	proc->runtime = runtime;
	proc->state = state;
	proc->pid = pid++;
	proc->state = PROCESS_READY_TO_RUN;
	proc->process_pagemap = vmm_new_pagemap();
	struct auxval auxv, ld_aux;
	struct vfs_node *node = vfs_get_node(vfs_root, path, true);
	const char *ld_path;

	if (!node ||
		!elf_load(proc->process_pagemap, node->resource, 0, &auxv, &ld_path))
		return false;

	if (ld_path) {
		struct vfs_node *ld_node = vfs_get_node(vfs_root, ld_path, true);

		if (!ld_node || !elf_load(proc->process_pagemap, ld_node->resource,
								  0x40000000, &ld_aux, NULL))
			return false;
	}

	uint64_t entry = ld_path == NULL ? auxv.at_entry : ld_aux.at_entry;

	proc->auxv = auxv;
	vec_init(&proc->child_processes);
	vec_init(&proc->process_threads);
	vec_init(&proc->waiter_processes);
	vec_push(&processes, proc);
	if (parent_process) {
		vec_push(&parent_process->child_processes, proc);
		if (parent_process->cwd != NULL) {
			proc->cwd = parent_process->cwd;
		} else {
			proc->cwd = vfs_root;
		}
		proc->umask = parent_process->umask;
		proc->mmap_anon_base = parent_process->mmap_anon_base;
	} else {
		proc->cwd = vfs_root;
		proc->umask = S_IWGRP | S_IWOTH;
		proc->mmap_anon_base = 0x80000000000;
	}

	for (int i = 0; i < 3; i++)
		fdnum_create_from_resource(proc, std_console_device, 0, i, true);

	thread_create((uintptr_t)entry, 0, 1, proc);

	spinlock_drop(process_lock);
	return true;
}

int64_t process_fork(struct process *proc, struct thread *thrd) {
	spinlock_acquire_or_wait(process_lock);
	struct process *fproc = kmalloc(sizeof(struct process));
	memcpy(fproc->name, proc->name, sizeof(proc->name));

	fproc->process_pagemap = vmm_fork_pagemap(proc->process_pagemap);

	fproc->mmap_anon_base = proc->mmap_anon_base;
	fproc->cwd = proc->cwd;
	fproc->umask = proc->umask;
	fproc->pid = pid++;
	fproc->parent_process = proc;

	vec_init(&fproc->child_processes);
	vec_init(&fproc->process_threads);
	vec_init(&fproc->waiter_processes);
	vec_push(&processes, fproc);

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

	thread_fork(thrd, fproc);

	fproc->state = PROCESS_READY_TO_RUN;

	spinlock_drop(process_lock);
	return fproc->pid;
}

void thread_create(uintptr_t pc_address, uint64_t arguments, bool user,
				   struct process *proc) {
	spinlock_acquire_or_wait(thread_lock);
	struct thread *thrd = kmalloc(sizeof(struct thread));
	thrd->tid = tid++;
	thrd->state = THREAD_READY_TO_RUN;
	thrd->runtime = proc->runtime;
	thrd->lock = 0;
	thrd->mother_proc = proc;
#if defined(__x86_64__)
	thrd->reg.rip = pc_address;
	thrd->reg.rdi = arguments;
	thrd->reg.rsp = (uint64_t)pmm_allocz(STACK_SIZE / PAGE_SIZE);
	thrd->stack = thrd->reg.rsp;
	if (user) {
		thrd->reg.cs = 0x23;
		thrd->reg.ss = 0x1b;
		mmap_range(proc->process_pagemap, VIRTUAL_STACK_ADDR - STACK_SIZE,
				   (uintptr_t)thrd->reg.rsp, STACK_SIZE, PROT_READ | PROT_WRITE,
				   MAP_ANONYMOUS);
		thrd->reg.rsp = VIRTUAL_STACK_ADDR;
		thrd->kernel_stack = (uint64_t)kmalloc(STACK_SIZE);
		thrd->kernel_stack += STACK_SIZE;
	} else {
		thrd->reg.cs = 0x08;
		thrd->reg.ss = 0x10;
		thrd->reg.rsp += STACK_SIZE;
		thrd->kernel_stack = thrd->reg.rsp;
	}
	thrd->reg.rflags = 0x202;

	thrd->fpu_storage =
		pmm_allocz(DIV_ROUNDUP(fpu_storage_size, PAGE_SIZE)) + MEM_PHYS_OFFSET;
	if (user) {
		fpu_restore(thrd->fpu_storage);
		uint16_t default_fcw = 0b1100111111;
		asm volatile("fldcw %0" ::"m"(default_fcw) : "memory");
		uint32_t default_mxcsr = 0b1111110000000;
		asm volatile("ldmxcsr %0" ::"m"(default_mxcsr) : "memory");
		fpu_save(thrd->fpu_storage);
		thrd->fs_base = 0;
		thrd->gs_base = 0;
		if (!proc->process_threads.length) {
			const char *argv[] = {proc->name, NULL};
			const char *envp[] = {"USER=root", NULL};
			struct auxval auxv = proc->auxv;

			uint64_t *stack = (uint64_t *)(thrd->stack + STACK_SIZE);

			// the stack structure address values are not accurate
			/*
			 * 	0x70000000000 - "USER=root\0" 	// envp[0][9]
			 * 	0x6fffffffff6 - proc->name 		// argv[0][255]
			 *	0x6fffffffef6 - 0x0, 0x0		// zeros
			 *	0x6fffffffee6 - AT_ENTRY
			 *	0x6fffffffede - 0x400789		// example values
			 *	0x6fffffffed6 - AT_PHDR
			 *	0x6fffffffece - 2
			 *	0x6fffffffec6 - AT_PHENT
			 *	0x6fffffffebe - 7
			 *	0x6fffffffeb6 - AT_PHNUM
			 *	0x6fffffffeae - 5
			 *	0x6fffffffea6 - 0x0 			// START OF ENVP
			 *	0x6fffffffe9e - 0x6fffffffff6	// pointer to envp[0][9] aka
			 *envp[0] 0x6fffffffe96 - 0x0				// START OF ARGV
			 *	0x6fffffffe8e - 0x6fffffffef6	// pointer to argv[0][0] aka
			 *argv[0] 0x6fffffffe86 - 1				// argc
			 */

			stack -= strlen(envp[0]) + 1;
			memcpy((void *)stack, envp[0], strlen(envp[0]) + 1);
			uint64_t address_difference =
				(thrd->stack + STACK_SIZE) - (uint64_t)stack;
			uint64_t addr_to_env =
				(uint64_t)VIRTUAL_STACK_ADDR - address_difference;

			stack -= strlen(argv[0]) + 1;
			memcpy((void *)stack, argv[0], strlen(argv[0]) + 1);

			address_difference = (thrd->stack + STACK_SIZE) - (uint64_t)stack;
			uint64_t addr_to_arg =
				(uint64_t)VIRTUAL_STACK_ADDR - address_difference;

			*(--stack) = 0;
			*(--stack) = 0;
			stack -= 2;
			stack[0] = 9;
			stack[1] = auxv.at_entry;
			stack -= 2;
			stack[0] = 3;
			stack[1] = auxv.at_phdr;
			stack -= 2;
			stack[0] = 4;
			stack[1] = auxv.at_phent;
			stack -= 2;
			stack[0] = 5;
			stack[1] = auxv.at_phnum;

			*(--stack) = 0;
			*(--stack) = addr_to_env;

			*(--stack) = 0;
			*(--stack) = addr_to_arg;

			*(--stack) = 1;

			address_difference = (thrd->stack + STACK_SIZE) - (uint64_t)stack;
			thrd->reg.rsp -= address_difference;
		}
	}
#endif
	thrd->sleeping_till = 0;

	vec_push(&threads, thrd);
	vec_push(&proc->process_threads, thrd);
	spinlock_drop(thread_lock);
}

void thread_fork(struct thread *pthrd, struct process *fproc) {
	spinlock_acquire_or_wait(thread_lock);
	struct thread *thrd = kmalloc(sizeof(struct thread));
	thrd->tid = tid++;
	thrd->state = THREAD_READY_TO_RUN;
	thrd->runtime = pthrd->runtime;
	thrd->lock = 0;
	thrd->mother_proc = fproc;
	memcpy(&thrd->reg, &pthrd->reg, sizeof(registers_t));
	thrd->kernel_stack = (uint64_t)kmalloc(STACK_SIZE);
	thrd->kernel_stack += STACK_SIZE;
#if defined(__x86_64__)
	thrd->reg.rax = 0;
	thrd->reg.rbx = 0;
	thrd->fs_base = pthrd->fs_base;
	thrd->gs_base = pthrd->gs_base;
	thrd->fpu_storage =
		pmm_allocz(DIV_ROUNDUP(fpu_storage_size, PAGE_SIZE)) + MEM_PHYS_OFFSET;

	memcpy(thrd->fpu_storage, pthrd->fpu_storage, fpu_storage_size);
#endif
	thrd->sleeping_till = 0;
	vec_push(&threads, thrd);
	vec_push(&fproc->process_threads, thrd);
	spinlock_drop(thread_lock);
}

bool process_execve(char *path, char **argv, char **envp) {
	spinlock_acquire_or_wait(process_lock);

	struct thread *thread = prcb_return_current_cpu()->running_thread;
	struct process *proc = thread->mother_proc;

	struct auxval auxv, ld_aux;
	struct vfs_node *node = vfs_get_node(proc->cwd, path, true);
	const char *ld_path;

	strncpy(proc->name, path, 256);

	if (!node) {
		return false;
	}

	struct pagemap *old_pagemap = proc->process_pagemap;
	proc->process_pagemap = vmm_new_pagemap();

	if (!elf_load(proc->process_pagemap, node->resource, 0, &auxv, &ld_path)) {
		proc->process_pagemap = old_pagemap;
		return false;
	}

	// HACK: ld_path for processes that dont depend on ld.so points to kmalloced
	// memory which is not null checking the first letter is '/' or not to know
	// if a program needs ld

	uint64_t entry = auxv.at_entry;

	if (ld_path && ld_path[0] == '/') {
		struct vfs_node *ld_node = vfs_get_node(vfs_root, ld_path, true);

		if (!ld_node || !elf_load(proc->process_pagemap, ld_node->resource,
								  0x40000000, &ld_aux, NULL)) {
			proc->process_pagemap = old_pagemap;
			return false;
		}
		entry = ld_aux.at_entry;
	}

	for (int i = 0; i < proc->process_threads.length; i++) {
		if (proc->process_threads.data[i] != thread) {
			thread_kill(proc->process_threads.data[i], 0);
		}
	}

	proc->mmap_anon_base = 0x80000000000;
	proc->state = PROCESS_READY_TO_RUN;

	// We no longer exist. There is no point in saving anything now.
	prcb_return_current_cpu()->running_thread = NULL;

	proc->auxv = auxv;

	thread_execve(proc, thread, entry, argv, envp);

	spinlock_drop(process_lock);

	vmm_switch_pagemap(kernel_pagemap);

	sched_resched_now();
	return false;
}

void process_kill(struct process *proc, bool crash) {
	spinlock_acquire_or_wait(process_lock);

	struct dead_process *dead_proc = kmalloc(sizeof(struct dead_process));
	dead_proc->pid = proc->pid;
	dead_proc->exit_code = proc->waitee.exit_code;
	dead_proc->parent_process = proc->parent_process;

	bool are_we_killing_ourselves = 0;
	if (prcb_return_current_cpu()->running_thread->mother_proc == proc)
		are_we_killing_ourselves = 1;

	for (int i = 0; i < proc->process_threads.length; i++)
		thread_kill(proc->process_threads.data[i], false);

	if (proc->parent_process)
		vec_remove(&proc->parent_process->child_processes, proc);

	// child processes are now owned by the init
	for (int i = 0; i < proc->child_processes.length; i++) {
		struct process *child_process = proc->child_processes.data[i];
		child_process->parent_process =
			processes.data[1]; // the init proc is the second process
							   // first one is the kernel
		vec_push(&processes.data[1]->child_processes, child_process);
		vec_remove(&proc->child_processes, child_process);
	}

	vec_deinit(&proc->child_processes);

	// processes waiting on this process can now stop waiting
	for (int i = 0; i < proc->waiter_processes.length; i++) {
		struct process *waiter_process = proc->waiter_processes.data[i];

		if (waiter_process == NULL)
			continue;

		waiter_process->state = PROCESS_READY_TO_RUN;
		if (are_we_killing_ourselves && !crash) {
			waiter_process->waitee.exit_code = proc->waitee.exit_code;
			waiter_process->waitee.was_it_killed = 0;
			dead_proc->was_it_killed = 0;

		} else {
			waiter_process->waitee.exit_code = -1;
			waiter_process->waitee.was_it_killed = 1;
			dead_proc->was_it_killed = 1;
		}
		waiter_process->waitee.pid = proc->pid;
		waiter_process->waitee.parent_process = proc->parent_process;
		vec_remove(&proc->waiter_processes, waiter_process);
	}

	vec_deinit(&proc->waiter_processes);
	vec_remove(&processes, proc);

	vec_push(&dead_processes, dead_proc);

	spinlock_drop(process_lock);

	if (are_we_killing_ourselves)
		sched_resched_now();
}

// TODO: This can be optimized way moreeeee

void thread_execve(struct process *proc, struct thread *thrd,
				   uintptr_t pc_address, char **argv, char **envp) {
	spinlock_acquire_or_wait(thread_lock);
	thrd->tid = tid++;
	thrd->state = THREAD_READY_TO_RUN;
	thrd->runtime = proc->runtime;
	thrd->lock = 0;
	thrd->mother_proc = proc;

#if defined(__x86_64__)
	thrd->reg.rip = pc_address;
	thrd->reg.rsp = (uint64_t)pmm_allocz(STACK_SIZE / PAGE_SIZE);
	thrd->stack = thrd->reg.rsp;
	thrd->reg.cs = 0x23;
	thrd->reg.ss = 0x1b;

	mmap_range(proc->process_pagemap, VIRTUAL_STACK_ADDR - STACK_SIZE,
			   (uintptr_t)thrd->reg.rsp, STACK_SIZE + 1, PROT_READ | PROT_WRITE,
			   MAP_ANONYMOUS);

	thrd->reg.rsp = VIRTUAL_STACK_ADDR;
	thrd->kernel_stack = (uint64_t)kmalloc(STACK_SIZE);
	thrd->kernel_stack += STACK_SIZE;

	thrd->reg.rflags = 0x202;

	thrd->fpu_storage =
		pmm_allocz(DIV_ROUNDUP(fpu_storage_size, PAGE_SIZE)) + MEM_PHYS_OFFSET;

	fpu_restore(thrd->fpu_storage);
	uint16_t default_fcw = 0b1100111111;
	asm volatile("fldcw %0" ::"m"(default_fcw) : "memory");
	uint32_t default_mxcsr = 0b1111110000000;
	asm volatile("ldmxcsr %0" ::"m"(default_mxcsr) : "memory");
	fpu_save(thrd->fpu_storage);

	thrd->fs_base = 0;
	thrd->gs_base = 0;

	struct auxval auxv = proc->auxv;

	uint64_t *stack = (uint64_t *)(thrd->stack + STACK_SIZE);

	int envp_len = 0;
	uint64_t address_difference = 0;

	uint8_t *stack_but_in_bytes = (uint8_t *)stack;

	for (envp_len = 0; envp[envp_len] != NULL; envp_len++) {
		stack_but_in_bytes -= (strlen(envp[envp_len]) + 1);
		memcpy((void *)stack_but_in_bytes + MEM_PHYS_OFFSET, envp[envp_len],
			   strlen(envp[envp_len]) + 1);
	}

	stack = (uint64_t *)stack_but_in_bytes;
	address_difference = (thrd->stack + STACK_SIZE) - (uint64_t)stack;
	uint64_t addr_to_env = (uint64_t)VIRTUAL_STACK_ADDR - address_difference;

	int argv_len;
	for (argv_len = 0; argv[argv_len] != NULL; argv_len++) {
		stack_but_in_bytes -= (strlen(argv[argv_len]) + 1);
		memcpy((void *)stack_but_in_bytes + MEM_PHYS_OFFSET, argv[argv_len],
			   strlen(argv[argv_len]) + 1);
	}

	stack = (uint64_t *)stack_but_in_bytes;
	address_difference = (thrd->stack + STACK_SIZE) - (uint64_t)stack;
	uint64_t addr_to_arg = (uint64_t)VIRTUAL_STACK_ADDR - address_difference;

	// alignments

	stack = (uintptr_t *)((uintptr_t)stack & ~(0b1111));
	if (((argv_len + envp_len + 1) & 1) != 0)
		stack--;

	stack = (uintptr_t *)((uintptr_t)stack + MEM_PHYS_OFFSET);

	*(--stack) = 0;
	*(--stack) = 0;
	stack -= 2;
	stack[0] = 9;
	stack[1] = auxv.at_entry;
	stack -= 2;
	stack[0] = 3;
	stack[1] = auxv.at_phdr;
	stack -= 2;
	stack[0] = 4;
	stack[1] = auxv.at_phent;
	stack -= 2;
	stack[0] = 5;
	stack[1] = auxv.at_phnum;

	*(--stack) = 0;

	stack -= envp_len;

	uint64_t offset = 0;
	for (int i = envp_len - 1; i >= 0; i--) {
		if (i != envp_len - 1) {
			offset += strlen(envp[i + 1]) + 1;
		}
		stack[i] = addr_to_env + offset;
	}

	*(--stack) = 0;

	stack -= argv_len;

	offset = 0;
	for (int i = argv_len - 1; i >= 0; i--) {
		if (i != argv_len - 1) {
			offset += strlen(argv[i + 1]) + 1;
		}
		stack[i] = addr_to_arg + offset;
	}

	*(--stack) = argv_len;

	stack = (uintptr_t *)((uintptr_t)stack - MEM_PHYS_OFFSET);

	address_difference = (thrd->stack + STACK_SIZE) - (uint64_t)stack;
	thrd->reg.rsp -= address_difference;
#endif

	spinlock_drop(thread_lock);
}

void thread_kill(struct thread *thrd, bool r) {
	spinlock_acquire_or_wait(thread_lock);
	if (thrd->mother_proc->pid <= 1) {
		if (thrd->mother_proc->process_threads.data[0] == thrd)
			panic("Attempted to kill init!\n");
	}
#if defined(__x86_64__)
	if (thrd->mother_proc != processes.data[0]) {
		pmm_free((void *)thrd->stack, STACK_SIZE / PAGE_SIZE);
		pmm_free((void *)thrd->fpu_storage, fpu_storage_size / PAGE_SIZE);
		kfree((void *)thrd->kernel_stack);
	} else {
		pmm_free((void *)thrd->stack, STACK_SIZE / PAGE_SIZE);
	}
#endif
	vec_remove(&thrd->mother_proc->process_threads, thrd);
	vec_remove(&threads, thrd);
	kfree(thrd);

	spinlock_drop(thread_lock);
	if (r) {
		sched_resched_now();
	}
}

void thread_sleep(struct thread *thrd, uint64_t ns) {
	cli();
	spinlock_acquire_or_wait(thread_lock);
	thrd->state = THREAD_SLEEPING;
	thrd->sleeping_till = timer_get_sleep_ns(ns);
	vec_push(&sleeping_threads, thrd);
	spinlock_drop(thread_lock);
	sti();

	while (prcb_return_current_cpu()->running_thread->sleeping_till >
		   timer_get_abs_count())
		;
}

void process_wait_on_another_process(struct process *waiter,
									 struct process *waitee) {
	waiter->state = PROCESS_WAITING_ON_ANOTHER_PROCESS;
	vec_push(&waitee->waiter_processes, waiter);
	sched_resched_now();
}

void process_wait_on_processes(struct process *waiter, process_vec_t waitees) {
	waiter->state = PROCESS_WAITING_ON_ANOTHER_PROCESS;
	for (int i = 0; i < waitees.length; i++) {
		struct process *waitee = waitees.data[i];
		vec_push(&waitee->waiter_processes, waiter);
	}
	sched_resched_now();
}
