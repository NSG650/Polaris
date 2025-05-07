#include <cpu/smp.h>
#include <debug/debug.h>
#include <sched/sched.h>
#include <sys/halt.h>
#include <sys/idt.h>
#include <sys/isr.h>
#include <sys/prcb.h>

void isr_install(void) {
	idt_set_gate(0, isr0, 0);
	idt_set_gate(1, isr1, 0);
	idt_set_gate(2, isr2, 0);
	idt_set_gate(3, isr3, 0);
	idt_set_gate(4, isr4, 0);
	idt_set_gate(5, isr5, 0);
	idt_set_gate(6, isr6, 0);
	idt_set_gate(7, isr7, 0);
	idt_set_gate(8, isr8, 0);
	idt_set_gate(9, isr9, 0);
	idt_set_gate(10, isr10, 0);
	idt_set_gate(11, isr11, 0);
	idt_set_gate(12, isr12, 0);
	idt_set_gate(13, isr13, 0);
	idt_set_gate(14, isr14, 2);
	idt_set_gate(15, isr15, 0);
	idt_set_gate(16, isr16, 0);
	idt_set_gate(17, isr17, 0);
	idt_set_gate(18, isr18, 0);
	idt_set_gate(19, isr19, 0);
	idt_set_gate(20, isr20, 0);
	idt_set_gate(21, isr21, 0);
	idt_set_gate(22, isr22, 0);
	idt_set_gate(23, isr23, 0);
	idt_set_gate(24, isr24, 0);
	idt_set_gate(25, isr25, 0);
	idt_set_gate(26, isr26, 0);
	idt_set_gate(27, isr27, 0);
	idt_set_gate(28, isr28, 0);
	idt_set_gate(29, isr29, 0);
	idt_set_gate(30, isr30, 0);
	idt_set_gate(31, isr31, 0);
	idt_set_gate(32, isr32, 0);
	idt_set_gate(33, isr33, 0);
	idt_set_gate(34, isr34, 0);
	idt_set_gate(35, isr35, 0);
	idt_set_gate(36, isr36, 0);
	idt_set_gate(37, isr37, 0);
	idt_set_gate(38, isr38, 0);
	idt_set_gate(39, isr39, 0);
	idt_set_gate(40, isr40, 0);
	idt_set_gate(41, isr41, 0);
	idt_set_gate(42, isr42, 0);
	idt_set_gate(43, isr43, 0);
	idt_set_gate(44, isr44, 0);
	idt_set_gate(45, isr45, 0);
	idt_set_gate(46, isr46, 0);
	idt_set_gate(47, isr47, 0);
	idt_set_gate(48, isr48, 1);
	idt_set_gate(49, isr49, 0);
	idt_set_gate(50, isr50, 0);
	idt_set_gate(51, isr51, 0);
	idt_set_gate(52, isr52, 0);
	idt_set_gate(53, isr53, 0);
	idt_set_gate(54, isr54, 0);
	idt_set_gate(55, isr55, 0);
	idt_set_gate(56, isr56, 0);
	idt_set_gate(57, isr57, 0);
	idt_set_gate(58, isr58, 0);
	idt_set_gate(59, isr59, 0);
	idt_set_gate(60, isr60, 0);
	idt_set_gate(61, isr61, 0);
	idt_set_gate(62, isr62, 0);
	idt_set_gate(63, isr63, 0);
	idt_set_gate(64, isr64, 0);
	idt_set_gate(65, isr65, 0);
	idt_set_gate(66, isr66, 0);
	idt_set_gate(67, isr67, 0);
	idt_set_gate(68, isr68, 0);
	idt_set_gate(69, isr69, 0);
	idt_set_gate(70, isr70, 0);
	idt_set_gate(71, isr71, 0);
	idt_set_gate(72, isr72, 0);
	idt_set_gate(73, isr73, 0);
	idt_set_gate(74, isr74, 0);
	idt_set_gate(75, isr75, 0);
	idt_set_gate(76, isr76, 0);
	idt_set_gate(77, isr77, 0);
	idt_set_gate(78, isr78, 0);
	idt_set_gate(79, isr79, 0);
	idt_set_gate(80, isr80, 0);
	idt_set_gate(81, isr81, 0);
	idt_set_gate(82, isr82, 0);
	idt_set_gate(83, isr83, 0);
	idt_set_gate(84, isr84, 0);
	idt_set_gate(85, isr85, 0);
	idt_set_gate(86, isr86, 0);
	idt_set_gate(87, isr87, 0);
	idt_set_gate(88, isr88, 0);
	idt_set_gate(89, isr89, 0);
	idt_set_gate(90, isr90, 0);
	idt_set_gate(91, isr91, 0);
	idt_set_gate(92, isr92, 0);
	idt_set_gate(93, isr93, 0);
	idt_set_gate(94, isr94, 0);
	idt_set_gate(95, isr95, 0);
	idt_set_gate(96, isr96, 0);
	idt_set_gate(97, isr97, 0);
	idt_set_gate(98, isr98, 0);
	idt_set_gate(99, isr99, 0);
	idt_set_gate(100, isr100, 0);
	idt_set_gate(101, isr101, 0);
	idt_set_gate(102, isr102, 0);
	idt_set_gate(103, isr103, 0);
	idt_set_gate(104, isr104, 0);
	idt_set_gate(105, isr105, 0);
	idt_set_gate(106, isr106, 0);
	idt_set_gate(107, isr107, 0);
	idt_set_gate(108, isr108, 0);
	idt_set_gate(109, isr109, 0);
	idt_set_gate(110, isr110, 0);
	idt_set_gate(111, isr111, 0);
	idt_set_gate(112, isr112, 0);
	idt_set_gate(113, isr113, 0);
	idt_set_gate(114, isr114, 0);
	idt_set_gate(115, isr115, 0);
	idt_set_gate(116, isr116, 0);
	idt_set_gate(117, isr117, 0);
	idt_set_gate(118, isr118, 0);
	idt_set_gate(119, isr119, 0);
	idt_set_gate(120, isr120, 0);
	idt_set_gate(121, isr121, 0);
	idt_set_gate(122, isr122, 0);
	idt_set_gate(123, isr123, 0);
	idt_set_gate(124, isr124, 0);
	idt_set_gate(125, isr125, 0);
	idt_set_gate(126, isr126, 0);
	idt_set_gate(127, isr127, 0);
	idt_set_gate(128, isr128, 0);
	idt_set_gate(129, isr129, 0);
	idt_set_gate(130, isr130, 0);
	idt_set_gate(131, isr131, 0);
	idt_set_gate(132, isr132, 0);
	idt_set_gate(133, isr133, 0);
	idt_set_gate(134, isr134, 0);
	idt_set_gate(135, isr135, 0);
	idt_set_gate(136, isr136, 0);
	idt_set_gate(137, isr137, 0);
	idt_set_gate(138, isr138, 0);
	idt_set_gate(139, isr139, 0);
	idt_set_gate(140, isr140, 0);
	idt_set_gate(141, isr141, 0);
	idt_set_gate(142, isr142, 0);
	idt_set_gate(143, isr143, 0);
	idt_set_gate(144, isr144, 0);
	idt_set_gate(145, isr145, 0);
	idt_set_gate(146, isr146, 0);
	idt_set_gate(147, isr147, 0);
	idt_set_gate(148, isr148, 0);
	idt_set_gate(149, isr149, 0);
	idt_set_gate(150, isr150, 0);
	idt_set_gate(151, isr151, 0);
	idt_set_gate(152, isr152, 0);
	idt_set_gate(153, isr153, 0);
	idt_set_gate(154, isr154, 0);
	idt_set_gate(155, isr155, 0);
	idt_set_gate(156, isr156, 0);
	idt_set_gate(157, isr157, 0);
	idt_set_gate(158, isr158, 0);
	idt_set_gate(159, isr159, 0);
	idt_set_gate(160, isr160, 0);
	idt_set_gate(161, isr161, 0);
	idt_set_gate(162, isr162, 0);
	idt_set_gate(163, isr163, 0);
	idt_set_gate(164, isr164, 0);
	idt_set_gate(165, isr165, 0);
	idt_set_gate(166, isr166, 0);
	idt_set_gate(167, isr167, 0);
	idt_set_gate(168, isr168, 0);
	idt_set_gate(169, isr169, 0);
	idt_set_gate(170, isr170, 0);
	idt_set_gate(171, isr171, 0);
	idt_set_gate(172, isr172, 0);
	idt_set_gate(173, isr173, 0);
	idt_set_gate(174, isr174, 0);
	idt_set_gate(175, isr175, 0);
	idt_set_gate(176, isr176, 0);
	idt_set_gate(177, isr177, 0);
	idt_set_gate(178, isr178, 0);
	idt_set_gate(179, isr179, 0);
	idt_set_gate(180, isr180, 0);
	idt_set_gate(181, isr181, 0);
	idt_set_gate(182, isr182, 0);
	idt_set_gate(183, isr183, 0);
	idt_set_gate(184, isr184, 0);
	idt_set_gate(185, isr185, 0);
	idt_set_gate(186, isr186, 0);
	idt_set_gate(187, isr187, 0);
	idt_set_gate(188, isr188, 0);
	idt_set_gate(189, isr189, 0);
	idt_set_gate(190, isr190, 0);
	idt_set_gate(191, isr191, 0);
	idt_set_gate(192, isr192, 0);
	idt_set_gate(193, isr193, 0);
	idt_set_gate(194, isr194, 0);
	idt_set_gate(195, isr195, 0);
	idt_set_gate(196, isr196, 0);
	idt_set_gate(197, isr197, 0);
	idt_set_gate(198, isr198, 0);
	idt_set_gate(199, isr199, 0);
	idt_set_gate(200, isr200, 0);
	idt_set_gate(201, isr201, 0);
	idt_set_gate(202, isr202, 0);
	idt_set_gate(203, isr203, 0);
	idt_set_gate(204, isr204, 0);
	idt_set_gate(205, isr205, 0);
	idt_set_gate(206, isr206, 0);
	idt_set_gate(207, isr207, 0);
	idt_set_gate(208, isr208, 0);
	idt_set_gate(209, isr209, 0);
	idt_set_gate(210, isr210, 0);
	idt_set_gate(211, isr211, 0);
	idt_set_gate(212, isr212, 0);
	idt_set_gate(213, isr213, 0);
	idt_set_gate(214, isr214, 0);
	idt_set_gate(215, isr215, 0);
	idt_set_gate(216, isr216, 0);
	idt_set_gate(217, isr217, 0);
	idt_set_gate(218, isr218, 0);
	idt_set_gate(219, isr219, 0);
	idt_set_gate(220, isr220, 0);
	idt_set_gate(221, isr221, 0);
	idt_set_gate(222, isr222, 0);
	idt_set_gate(223, isr223, 0);
	idt_set_gate(224, isr224, 0);
	idt_set_gate(225, isr225, 0);
	idt_set_gate(226, isr226, 0);
	idt_set_gate(227, isr227, 0);
	idt_set_gate(228, isr228, 0);
	idt_set_gate(229, isr229, 0);
	idt_set_gate(230, isr230, 0);
	idt_set_gate(231, isr231, 0);
	idt_set_gate(232, isr232, 0);
	idt_set_gate(233, isr233, 0);
	idt_set_gate(234, isr234, 0);
	idt_set_gate(235, isr235, 0);
	idt_set_gate(236, isr236, 0);
	idt_set_gate(237, isr237, 0);
	idt_set_gate(238, isr238, 0);
	idt_set_gate(239, isr239, 0);
	idt_set_gate(240, isr240, 0);
	idt_set_gate(241, isr241, 0);
	idt_set_gate(242, isr242, 0);
	idt_set_gate(243, isr243, 0);
	idt_set_gate(244, isr244, 0);
	idt_set_gate(245, isr245, 0);
	idt_set_gate(246, isr246, 0);
	idt_set_gate(247, isr247, 0);
	idt_set_gate(248, isr248, 0);
	idt_set_gate(249, isr249, 0);
	idt_set_gate(250, isr250, 0);
	idt_set_gate(251, isr251, 0);
	idt_set_gate(252, isr252, 0);
	idt_set_gate(253, isr253, 0);
	idt_set_gate(254, isr254, 0);
	idt_set_gate(255, isr255, 0);
	idt_reload();
}

static const char *isr_exception_messages[] = {"Divide by zero",
											   "Debug",
											   "NMI",
											   "Breakpoint",
											   "Overflow",
											   "Bound Range Exceeded",
											   "Invalid Opcode",
											   "Device Not Available",
											   "Double fault",
											   "Co-processor Segment Overrun",
											   "Invalid TSS",
											   "Segment not present",
											   "Stack-Segment Fault",
											   "GPF",
											   "Page Fault",
											   "Reserved",
											   "x87 Floating Point Exception",
											   "alignment check",
											   "Machine check",
											   "SIMD floating-point exception",
											   "Virtualization Exception",
											   "Deadlock",
											   "Reserved",
											   "Reserved",
											   "Reserved",
											   "Reserved",
											   "Reserved",
											   "Reserved",
											   "Reserved",
											   "Reserved",
											   "Reserved",
											   "Security Exception",
											   "Reserved",
											   "Triple Fault",
											   "FPU error"};

static event_handlers_t event_handlers[256] = {NULL};

void isr_register_handler(int n, void *handler) {
	event_handlers[n] = handler;
}

void isr_handle(registers_t *r) {
	if (r->cs & 0x3) {
		swapgs();
	}

	if (r->isrNumber < 256 && event_handlers[r->isrNumber] != NULL)
		return event_handlers[r->isrNumber](r);

	if (r->isrNumber < 32) {
		if (r->cs & 0x3) {
			struct thread *thrd = sched_get_running_thread();
			kprintf("Killing user thread tid %d under process %s for exception "
					"%s\n",
					thrd->tid, thrd->mother_proc->name,
					isr_exception_messages[r->isrNumber]);
			kprintf("User thread crashed at address: %p\n", r->rip);
			thread_kill(thrd, true);
		} else {
			halt_other_cpus();
			kprintffos(0, "AH! UNHANDLED EXCEPTION!\n");
			kprintffos(0, "RIP: %p RBP: %p RSP: %p\n", r->rip, r->rbp, r->rsp);
			kprintffos(0, "RAX: %p RBX: %p RCX: %p\n", r->rax, r->rbx, r->rcx);
			kprintffos(0, "RDX: %p RDI: %p RSI: %p\n", r->rdx, r->rdi, r->rsi);
			kprintffos(0, "R8 : %p R9 : %p R10: %p\n", r->r8, r->r9, r->r10);
			kprintffos(0, "R11: %p R12: %p R13: %p\n", r->r11, r->r12, r->r13);
			kprintffos(0, "R14: %p R15: %p ERR: 0b%b\n", r->r14, r->r15,
					   r->errorCode);
			kprintffos(0, "CS : %p SS : %p RFLAGS: %p\n", r->cs, r->ss,
					   r->rflags);
			kprintffos(0, "FS: %p UGS: %p KGS: %p\n", read_fs_base(),
					   read_user_gs(), read_kernel_gs());
			panic_((void *)r->rip, (void *)r->rbp, "Unhandled Exception: %s\n",
				   isr_exception_messages[r->isrNumber]);
		}
	}

	if (r->cs & 0x3) {
		swapgs();
	}
}
