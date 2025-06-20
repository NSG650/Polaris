#ifndef SCHED_H
#define SCHED_H

#include <klibc/elf.h>
#include <locks/spinlock.h>
#include <sched/sched_types.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__x86_64__)
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <reg.h>
#include <sys/apic.h>
#include <sys/elf.h>
void resched(registers_t *reg);
void thread_setup_context(struct thread *thrd, uintptr_t pc_address,
						  uint64_t arguments, bool user);
void thread_setup_context_from_user(struct thread *thrd, uintptr_t pc_address,
									uintptr_t sp);
void thread_setup_context_for_execve(struct thread *thrd, uintptr_t pc_address,
									 char **argv, char **envp);
void thread_fork_context(struct thread *thrd, struct thread *fthrd);
void thread_destroy_context(struct thread *thrd);
void process_setup_context(struct process *proc, bool user);
void process_fork_context(struct process *proc, struct process *fproc);
void process_destroy_context(struct process *proc);
#endif

extern struct thread *thread_list;
extern struct process *process_list;
extern struct thread *threads_on_the_death_row;
extern struct thread *sleeping_threads;
extern struct process *processes_on_the_death_row;
extern struct resource *std_console_device;

void sched_yield(bool save);
struct thread *sched_get_next_thread(struct thread *thrd);
void sched_init(uint64_t args);
void sched_add_thread_to_list(struct thread **thrd_list, struct thread *thrd);
void sched_remove_thread_from_list(struct thread **thrd_list,
								   struct thread *thrd);
void sched_add_process_to_list(struct process **proc_list,
							   struct process *proc);
void sched_remove_process_from_list(struct process **proc_list,
									struct process *proc);
void process_create(char *name, uint8_t state, uint64_t runtime,
					uintptr_t pc_address, uint64_t arguments, bool user,
					struct process *parent_process);
bool process_create_elf(char *name, uint8_t state, uint64_t runtime, char *path,
						struct process *parent_process);
void process_kill(struct process *proc, bool crash);
int64_t process_fork(struct process *proc, struct thread *thrd);
bool process_execve(char *path, char **argv, char **envp);
void thread_create(uintptr_t pc_address, uint64_t arguments, bool user,
				   struct process *proc);
void thread_execve(struct process *proc, struct thread *thrd,
				   uintptr_t pc_address, char **argv, char **envp);
void thread_kill(struct thread *thrd, bool reschedule);
void thread_sleep(struct thread *thrd, uint64_t ns);
void thread_fork(struct thread *pthrd, struct process *fproc);
void syscall_prctl(struct syscall_arguments *args);

void process_wait_on_another_process(struct process *waiter,
									 struct process *waitee);

void process_wait_on_processes(struct process *waiter, process_vec_t *waitees);

#if defined(__x86_64__)
static inline struct thread *sched_get_running_thread(void) {
	struct thread *ret = NULL;
	asm volatile("mov %0, qword ptr gs:[24]" : "=r"(ret)::"memory");
	return ret;
}
#endif

#endif
