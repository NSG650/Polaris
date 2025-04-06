#include <cpu/smp.h>
#include <debug/debug.h>
#include <klibc/mem.h>
#include <sys/halt.h>
#include <sys/isr.h>
#include <sys/prcb.h>

static void backtrace_dump(registers_t *reg) {
	kprintffos(0, "============   Backtrace  ==========\n");

	kprintffos(0, "-> %p\n", reg->rip);

	uintptr_t *rbp = (uintptr_t *)reg->rbp;

	if (rbp == NULL)
		asm volatile("mov %%rbp, %0" : "=g"(rbp)::"memory");

	if (rbp == NULL)
		return;

	for (;;) {
		uintptr_t *old_rbp = (uintptr_t *)rbp[0];
		uintptr_t *rip = (uintptr_t *)rbp[1];

		if (rip == NULL || old_rbp == NULL)
			break;

		kprintffos(0, "%p\n", rip);

		rbp = old_rbp;
	}

	kprintffos(0, "============ End of dumps ==========\n");
}

static void register_dump(registers_t *reg) {
	kprintffos(0, "========= Register dumps =========\n");
	kprintffos(0, "RIP: %p RBP: %p RSP: %p\n", reg->rip, reg->rbp, reg->rsp);
	kprintffos(0, "RAX: %p RBX: %p RCX: %p\n", reg->rax, reg->rbx, reg->rcx);
	kprintffos(0, "RDX: %p RDI: %p RSI: %p\n", reg->rdx, reg->rdi, reg->rsi);
	kprintffos(0, "R8 : %p R9 : %p R10: %p\n", reg->r8, reg->r9, reg->r10);
	kprintffos(0, "R11: %p R12: %p R13: %p\n", reg->r11, reg->r12, reg->r13);
	kprintffos(0, "R14: %p R15: %p\n", reg->r14, reg->r15);
	kprintffos(0, "CS : %p SS : %p RFLAGS: %p\n", reg->cs, reg->ss,
			   reg->rflags);
	kprintffos(0, "FS: %p UGS: %p KGS: %p\n", read_fs_base(), read_user_gs(),
			   read_kernel_gs());

	kprintffos(0, "============ End of dumps ==========\n");
}

static void tss_dump(void) {
	kprintffos(0, "============   TSS Dumps  ==========\n");

	kprintffos(0, "tss.rsp0: %p\n", prcb_return_current_cpu()->cpu_tss.rsp0);
	kprintffos(0, "tss.rsp1: %p\n", prcb_return_current_cpu()->cpu_tss.rsp1);
	kprintffos(0, "tss.rsp2: %p\n", prcb_return_current_cpu()->cpu_tss.rsp2);

	kprintffos(0, "tss.ist1: %p\n", prcb_return_current_cpu()->cpu_tss.ist1);
	kprintffos(0, "tss.ist2: %p\n", prcb_return_current_cpu()->cpu_tss.ist2);
	kprintffos(0, "tss.ist3: %p\n", prcb_return_current_cpu()->cpu_tss.ist3);
	kprintffos(0, "tss.ist4: %p\n", prcb_return_current_cpu()->cpu_tss.ist4);
	kprintffos(0, "tss.ist5: %p\n", prcb_return_current_cpu()->cpu_tss.ist5);
	kprintffos(0, "tss.ist6: %p\n", prcb_return_current_cpu()->cpu_tss.ist6);
	kprintffos(0, "tss.ist7: %p\n", prcb_return_current_cpu()->cpu_tss.ist7);

	kprintffos(0, "tss.iomap_base: %p\n",
			   prcb_return_current_cpu()->cpu_tss.iomap_base);

	kprintffos(0, "============ End of dumps ==========\n");
}

static void prcb_dump(void) {
	kprintffos(0, "============  PRCB Dumps  ==========\n");

	kprintffos(0, "prcb->cpu_number: %u\n",
			   prcb_return_current_cpu()->cpu_number);
	kprintffos(0, "prcb->kernel_stack: %p\n",
			   prcb_return_current_cpu()->kernel_stack);
	kprintffos(0, "prcb->user_stack: %p\n",
			   prcb_return_current_cpu()->user_stack);
	kprintffos(0, "prcb->running_thread: %p\n",
			   prcb_return_current_cpu()->running_thread);
	kprintffos(0, "prcb->sched_ticks: %p\n",
			   prcb_return_current_cpu()->user_stack);

	kprintffos(0, "prcb->lapic_id: %u\n", prcb_return_current_cpu()->lapic_id);
	kprintffos(0, "prcb->fpu_storage_size: %p\n",
			   prcb_return_current_cpu()->fpu_storage_size);
	kprintffos(0, "prcb->fpu_save: %p\n", prcb_return_current_cpu()->fpu_save);
	kprintffos(0, "prcb->fpu_restore: %p\n",
			   prcb_return_current_cpu()->fpu_restore);

	kprintffos(0, "============ End of dumps ==========\n");
}

void breakpoint_handler(registers_t *reg) {
	if (reg->cs & 0x3) {
		kprintffos(0, "Breakpoint hit in user!\n");
		thread_kill(prcb_return_current_cpu()->running_thread, true);
		return;
	} else {
		kprintffos(0, "Breakpoint hit in kernel!\n");
	}

	pause_other_cpus();

	kprintffos(0, "=========== Start of dumps =========\n");
	kprintffos(0, "Breakpoint hit on CPU%u\n",
			   prcb_return_current_cpu()->cpu_number);

	kprintffos(0, "========= Register dumps =========\n");
	kprintffos(0, "RIP: %p RBP: %p RSP: %p\n", reg->rip, reg->rbp, reg->rsp);
	kprintffos(0, "RAX: %p RBX: %p RCX: %p\n", reg->rax, reg->rbx, reg->rcx);
	kprintffos(0, "RDX: %p RDI: %p RSI: %p\n", reg->rdx, reg->rdi, reg->rsi);
	kprintffos(0, "R8 : %p R9 : %p R10: %p\n", reg->r8, reg->r9, reg->r10);
	kprintffos(0, "R11: %p R12: %p R13: %p\n", reg->r11, reg->r12, reg->r13);
	kprintffos(0, "R14: %p R15: %p\n", reg->r14, reg->r15);
	kprintffos(0, "CS : %p SS : %p RFLAGS: %p\n", reg->cs, reg->ss,
			   reg->rflags);
	kprintffos(0, "FS: %p UGS: %p KGS: %p\n", read_fs_base(), read_user_gs(),
			   read_kernel_gs());

	kprintffos(0, "============ End of dumps ==========\n");

	char option = 0;

	while (option != 'C') {
		kprintffos(0, "(C)ontinue, Dump (P)RCB, Dump (R)egisters, Dump (T)SS, "
					  "(B)acktrace?\n");
		option = serial_getchar();
		serial_putchar('\n');
		switch (option) {
			case 'P':
				prcb_dump();
				break;
			case 'R':
				register_dump(reg);
				break;
			case 'T':
				tss_dump();
				break;
			case 'B':
				backtrace_dump(reg);
			default:
				break;
		}
	}

	unpause_other_cpus();
}
