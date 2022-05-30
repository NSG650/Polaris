#include <sched/syscall.h>
#include <sys/isr.h>

static void syscall_handler(registers_t *reg) {
	struct syscall_arguments args = {
		.syscall_nr = reg->rax,
		.args0 = reg->rdi,
		.args1 = reg->rsi,
		.args2 = reg->rdx,
		.args3 = reg->r10
	};
	syscall_handle(&args);
}

void syscall_install_handler(void) {
	isr_register_handler(0x80, syscall_handler);
}
