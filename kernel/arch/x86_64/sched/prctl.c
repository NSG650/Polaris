#include <sched/sched.h>
#include <cpu/smp.h>
#include <sys/prcb.h>
#include <errno.h>

#define ARCH_SET_GS 0x1001
#define ARCH_SET_FS 0x1002
#define ARCH_GET_FS 0x1003
#define ARCH_GET_GS 0x1004

void syscall_prctl(struct syscall_arguments *args) {
	int option = (int)args->args0;
	uint64_t arg2 = args->args1;
	switch (option) {
		case ARCH_SET_GS:
			set_user_gs(arg2);
			break;
		case ARCH_GET_GS:
			args->ret = read_user_gs();
			break;
		case ARCH_SET_FS:
			set_fs_base(arg2);
			break;
		case ARCH_GET_FS:
			args->ret = read_fs_base();
			break;
		default:
			errno = -EINVAL;
			break;
	}

}
