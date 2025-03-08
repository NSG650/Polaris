#include <cpu/smp.h>
#include <debug/debug.h>
#include <errno.h>
#include <sched/sched.h>
#include <sys/prcb.h>

#define ARCH_SET_GS 0x1001
#define ARCH_SET_FS 0x1002
#define ARCH_GET_FS 0x1003
#define ARCH_GET_GS 0x1004

void syscall_prctl(struct syscall_arguments *args) {
	int option = (int)args->args0;
	uint64_t value = args->args1;
	switch (option) {
		case ARCH_SET_GS: {
			sched_get_running_thread()->gs_base = value;
			// set_user_gs(prcb_return_current_cpu()->running_thread->gs_base);
			break;
		}
		case ARCH_GET_GS:
			args->ret = read_user_gs();
			break;
		case ARCH_SET_FS: {
			sched_get_running_thread()->fs_base = value;
			set_fs_base(sched_get_running_thread()->fs_base);
			break;
		}
		case ARCH_GET_FS:
			args->ret = read_fs_base();
			break;
		default:
			errno = EINVAL;
			break;
	}
}
