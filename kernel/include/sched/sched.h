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
#include <sys/elf.h>
void resched(registers_t *reg);
#endif

extern process_vec_t processes;
extern thread_vec_t threads;
extern thread_vec_t sleeping_threads;

void sched_resched_now(void);
int sched_get_next_thread(int index);
void sched_init(uint64_t args);
void process_create(char *name, uint8_t state, uint64_t runtime,
					uintptr_t pc_address, uint64_t arguments, bool user,
					struct process *parent_process);
void process_create_elf(char *name, uint8_t state, uint64_t runtime, char *path,
						struct process *parent_process);
void process_kill(struct process *proc);
int process_fork(struct process *proc, struct thread *thrd);
bool process_execve(char *path, char **argv, char **envp);
void thread_create(uintptr_t pc_address, uint64_t arguments, bool user,
				   struct process *proc);
void thread_kill(struct thread *thrd, bool r);
void thread_sleep(struct thread *thrd, uint64_t ns);
void thread_fork(struct thread *pthrd, struct process *fproc);
void syscall_prctl(struct syscall_arguments *args);

#endif
