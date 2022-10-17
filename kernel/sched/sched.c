#include <asm/asm.h>
#include <cpu/smp.h>
#include <debug/debug.h>
#include <errno.h>
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

lock_t sched_lock = 0;
bool sched_runit = false;

thread_vec_t threads;
thread_vec_t sleeping_threads;
process_vec_t processes;

int64_t pid = -1;
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

		if (thread->state != THREAD_READY_TO_RUN)
			index++;

		if (spinlock_acquire(thread->lock))
			return index;
		index++;
	}
	return -1;
}

void syscall_kill(struct syscall_arguments *args) {
	struct process *proc = sched_pid_to_process(args->args0);
	process_kill(proc);
}

void syscall_exit(struct syscall_arguments *args) {
	(void)args;
	process_kill(prcb_return_current_cpu()->running_thread->mother_proc);
}

void syscall_getpid(struct syscall_arguments *args) {
	args->ret = prcb_return_current_cpu()->running_thread->mother_proc->pid;
}

void syscall_getppid(struct syscall_arguments *args) {
	args->ret = prcb_return_current_cpu()
					->running_thread->mother_proc->parent_process->pid;
}

void syscall_nanosleep(struct syscall_arguments *args) {
	struct __kernel_timespec *from_user =
		(struct __kernel_timespec *)args->args0;
	uint64_t seconds_to_ns = from_user->tv_sec * 1000000000;
	uint64_t total_sleep = seconds_to_ns + from_user->tv_nsec;
	thread_sleep(prcb_return_current_cpu()->running_thread, total_sleep);
}

void syscall_fork(struct syscall_arguments *args) {
	struct thread *running_thread = prcb_return_current_cpu()->running_thread;
	struct process *running_process = running_thread->mother_proc;
	args->ret = process_fork(running_process, running_thread);
}

void syscall_execve(struct syscall_arguments *args) {
	/*char *path = (char *)args->args0;
	char **argv = (char **)args->args1;
	char **envp = (char**)args->args2;
	kprintf("path = %s\n", path);
	for (int i = 0; argv[i] != NULL; i++) {
		kprintf("argv[%d] = %s\n", i, argv[i]);
	}
	for (int i = 0; envp[i] != NULL; i++) {
		kprintf("envp[%d] = %s\n", i, envp[i]);
	}
	args->ret = -1;*/
	if (!process_execve((char *)args->args0, (char **)args->args1,
						(char **)args->args2))
		args->ret = -1;
}

void sched_init(uint64_t args) {
	kprintf("SCHED: Creating kernel thread\n");
	vec_init(&threads);
	vec_init(&processes);
	vec_init(&threads);
	syscall_register_handler(0x27, syscall_getpid);
	syscall_register_handler(0x67, syscall_puts);
	syscall_register_handler(0x6e, syscall_getppid);
	syscall_register_handler(0x3c, syscall_exit);
	syscall_register_handler(0x3e, syscall_kill);
	syscall_register_handler(0x9d, syscall_prctl);
	syscall_register_handler(0x23, syscall_nanosleep);
	syscall_register_handler(0x39, syscall_fork);
	syscall_register_handler(0x3b, syscall_execve);
	process_create("kernel_tasks", 0, 5000, (uintptr_t)kernel_main, args, 0,
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
#if defined(__x86_64__)
	if (user) {
		proc->process_pagemap = vmm_new_pagemap();
	} else {
		proc->process_pagemap = kernel_pagemap;
	}
#endif
	vec_init(&proc->process_threads);
	vec_init(&proc->child_processes);
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

void process_create_elf(char *name, uint8_t state, uint64_t runtime, char *path,
						struct process *parent_process) {
	spinlock_acquire_or_wait(process_lock);
	struct process *proc = kmalloc(sizeof(struct process));
	strncpy(proc->name, name, 256);
	proc->runtime = runtime;
	proc->state = state;
	proc->pid = pid++;
	proc->state = PROCESS_READY_TO_RUN;
	proc->process_pagemap = vmm_new_pagemap();

	struct auxval auxv;
	struct vfs_node *node = vfs_get_node(vfs_root, path, true);
	elf_load(proc->process_pagemap, node->resource, 0, &auxv, NULL);

	vec_init(&proc->child_processes);
	vec_init(&proc->process_threads);
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
	thread_create((uintptr_t)auxv.at_entry, 0, 1, proc);
	spinlock_drop(process_lock);
}

int process_fork(struct process *proc, struct thread *thrd) {
	spinlock_acquire_or_wait(process_lock);
	struct process *fproc = kmalloc(sizeof(struct process));
	memcpy(fproc->name, proc->name, sizeof(proc->name));

	fproc->process_pagemap = vmm_fork_pagemap(proc->process_pagemap);

	fproc->mmap_anon_base = proc->mmap_anon_base;
	fproc->cwd = proc->cwd;
	fproc->umask = proc->umask;
	fproc->pid = pid++;

	vec_init(&proc->child_processes);
	vec_init(&proc->process_threads);
	vec_push(&processes, proc);

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
	proc->state = PROCESS_READY_TO_RUN;
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
	thrd->reg = pthrd->reg;
	thrd->kernel_stack = (uint64_t)kmalloc(STACK_SIZE);
	thrd->kernel_stack += STACK_SIZE;
#if defined(__x86_64__)
	thrd->reg.rax = 0;
	thrd->reg.rbx = 0;
	thrd->fs_base = read_fs_base();
	thrd->gs_base = read_kernel_gs();
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

	proc->state = PROCESS_BLOCKED;

	struct pagemap *new_pagemap = vmm_new_pagemap();
	struct auxval auxv, ld_auxv;
	char *ld_path;

	proc->cwd = vfs_root;
	struct vfs_node *node = vfs_get_node(proc->cwd, path, true);
	if (node == NULL || !elf_load(new_pagemap, node->resource, 0, &auxv, &ld_path))
		return false;

	// We will need this later
	/*struct vfs_node *ld_node = vfs_get_node(proc->cwd, ld_path, true);
	if (ld_node == NULL || !elf_load(new_pagemap, ld_node->resource, 0x40000000, &ld_auxv, NULL))
		return false;*/

	struct pagemap *old_pagemap = proc->process_pagemap;

	proc->process_pagemap = new_pagemap;
	proc->mmap_anon_base = 0x80000000000;

	int stuff = proc->process_threads.length;
	for (int i = 0; i < stuff; i++)
		thread_kill(proc->process_threads.data[i], false);

	// Child processes are now owned by the init
	/*for (int i = 0; i < proc->child_processes.length; i++) {
		struct process *child_process = proc->child_processes.data[i];
		child_process->parent_process = processes.data[1];
		vec_push(&processes.data[1]->child_processes, child_process);
	}*/

	uint64_t entry = ld_path == NULL ? auxv.at_entry : ld_auxv.at_entry;

	vfs_pathname(node, proc->name, sizeof(proc->name) - 1);

	// if(!thread_execve(proc, header->e_entry, argv, envp, &aux)) {
	//	spinlock_drop(process_lock);
	//	return false;
	// }

	thread_create(entry, 0, 1, proc);

	proc->state = PROCESS_READY_TO_RUN;
	vmm_switch_pagemap(kernel_pagemap);

	spinlock_drop(process_lock);

	//vmm_destroy_pagemap(old_pagemap);
	sched_resched_now();
	__builtin_unreachable();
	return true;
}

void process_kill(struct process *proc) {
	spinlock_acquire_or_wait(process_lock);
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
	}

	vec_remove(&processes, proc);
	spinlock_drop(process_lock);
	sched_resched_now();
}

bool thread_execve(struct process *proc, uintptr_t pc_address, char **argv,
				   char **envp, struct auxval *aux) {
	spinlock_acquire_or_wait(thread_lock);

	// the crazy execve stuff

	// first get the length of the envp and argv
	size_t argc = 0;
	size_t envc = 0;

	char did_we_find_null = 0;

	for (size_t i = 0; i < 128; i++) {
		if (!argv[i]) {
			did_we_find_null++;
			break;
		}
		argc++;
	}

	for (size_t i = 0; i < 128; i++) {
		if (!envp[i]) {
			did_we_find_null++;
			break;
		}
		envc++;
	}

	// its not a valid argv or envp
	if (did_we_find_null < 2) {
		spinlock_drop(thread_lock);
		return false;
	}

	struct thread *thrd = kmalloc(sizeof(struct thread));
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
			   (uintptr_t)thrd->reg.rsp, STACK_SIZE, PROT_READ | PROT_WRITE,
			   MAP_ANONYMOUS);
	thrd->reg.rsp = VIRTUAL_STACK_ADDR;
	thrd->kernel_stack = (uint64_t)kmalloc(STACK_SIZE);
	thrd->kernel_stack += STACK_SIZE;
	thrd->reg.rflags = 0x202;

	// :woozy_face:

	kprintf("rsp: %p\n", thrd->reg.rsp);

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
#endif
	thrd->state = THREAD_READY_TO_RUN;
	thrd->sleeping_till = 0;
	vec_push(&threads, thrd);
	vec_push(&proc->process_threads, thrd);
	spinlock_drop(thread_lock);
	return true;
}

void thread_kill(struct thread *thrd, bool r) {
	spinlock_acquire_or_wait(thread_lock);
	if (thrd->mother_proc->pid < 1) {
		if (thrd->mother_proc->process_threads.data[0] == thrd)
			panic("Attempted to kill init!\n");
	}
#if defined(__x86_64__)
	pmm_free((void *)thrd->stack, STACK_SIZE / PAGE_SIZE);
	// pmm_free((void *)thrd->fpu_storage, DIV_ROUNDUP(fpu_storage_size,
	// PAGE_SIZE));
	kfree((void *)thrd->kernel_stack);
#endif
	vec_remove(&thrd->mother_proc->process_threads, thrd);
	vec_remove(&threads, thrd);
	kfree(thrd);
	spinlock_drop(thread_lock);
	if (r)
		sched_resched_now();
}

void thread_sleep(struct thread *thrd, uint64_t ns) {
	cli();
	spinlock_acquire_or_wait(thread_lock);
	thrd->state = THREAD_SLEEPING;
	thrd->sleeping_till = timer_get_sleep_ns(ns);
	spinlock_drop(thread_lock);
	sti();
	if (prcb_return_current_cpu()->running_thread == thrd)
		sched_resched_now();
}
