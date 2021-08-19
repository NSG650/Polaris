#include "isr.h"
#include "../kernel/panic.h"
#include "../klibc/printf.h"
#include "idt.h"
#include "ports.h"

void isr_install(void) {
	set_idt_gate(0, (uint64_t)isr0);
	set_idt_gate(1, (uint64_t)isr1);
	set_idt_gate(2, (uint64_t)isr2);
	set_idt_gate(3, (uint64_t)isr3);
	set_idt_gate(4, (uint64_t)isr4);
	set_idt_gate(5, (uint64_t)isr5);
	set_idt_gate(6, (uint64_t)isr6);
	set_idt_gate(7, (uint64_t)isr7);
	set_idt_gate(8, (uint64_t)errorIsr8);
	set_idt_gate(9, (uint64_t)isr9);
	set_idt_gate(10, (uint64_t)errorIsr10);
	set_idt_gate(11, (uint64_t)errorIsr11);
	set_idt_gate(12, (uint64_t)errorIsr12);
	set_idt_gate(13, (uint64_t)errorIsr13);
	set_idt_gate(14, (uint64_t)errorIsr14);
	set_idt_gate(15, (uint64_t)isr15);
	set_idt_gate(16, (uint64_t)isr16);
	set_idt_gate(17, (uint64_t)isr17);
	set_idt_gate(18, (uint64_t)isr18);
	set_idt_gate(19, (uint64_t)isr19);
	set_idt_gate(20, (uint64_t)isr20);
	set_idt_gate(21, (uint64_t)isr21);
	set_idt_gate(22, (uint64_t)isr22);
	set_idt_gate(23, (uint64_t)isr23);
	set_idt_gate(24, (uint64_t)isr24);
	set_idt_gate(25, (uint64_t)isr25);
	set_idt_gate(26, (uint64_t)isr26);
	set_idt_gate(27, (uint64_t)isr27);
	set_idt_gate(28, (uint64_t)isr28);
	set_idt_gate(29, (uint64_t)isr29);
	set_idt_gate(30, (uint64_t)isr30);
	set_idt_gate(31, (uint64_t)isr31);

	// Remap the PIC
	port_byte_out(0x20, 0x11);
	port_byte_out(0xA0, 0x11);
	port_byte_out(0x21, 0x20);
	port_byte_out(0xA1, 0x28);
	port_byte_out(0x21, 0x04);
	port_byte_out(0xA1, 0x02);
	port_byte_out(0x21, 0x01);
	port_byte_out(0xA1, 0x01);
	port_byte_out(0x21, 0x0);
	port_byte_out(0xA1, 0x0);

	set_idt_gate(32, (uint64_t)isr32);
	set_idt_gate(33, (uint64_t)isr33);
	set_idt_gate(34, (uint64_t)isr34);
	set_idt_gate(35, (uint64_t)isr35);
	set_idt_gate(36, (uint64_t)isr36);
	set_idt_gate(37, (uint64_t)isr37);
	set_idt_gate(38, (uint64_t)isr38);
	set_idt_gate(39, (uint64_t)isr39);
	set_idt_gate(40, (uint64_t)isr40);
	set_idt_gate(41, (uint64_t)isr41);
	set_idt_gate(42, (uint64_t)isr42);
	set_idt_gate(43, (uint64_t)isr43);
	set_idt_gate(44, (uint64_t)isr44);
	set_idt_gate(45, (uint64_t)isr45);
	set_idt_gate(46, (uint64_t)isr46);
	set_idt_gate(47, (uint64_t)isr47);
	set_idt_gate(48, (uint64_t)isr48);
	set_idt_gate(49, (uint64_t)isr49);
	set_idt_gate(50, (uint64_t)isr50);
	set_idt_gate(51, (uint64_t)isr51);
	set_idt_gate(52, (uint64_t)isr52);
	set_idt_gate(53, (uint64_t)isr53);
	set_idt_gate(54, (uint64_t)isr54);
	set_idt_gate(55, (uint64_t)isr55);
	set_idt_gate(56, (uint64_t)isr56);
	set_idt_gate(57, (uint64_t)isr57);
	set_idt_gate(58, (uint64_t)isr58);
	set_idt_gate(59, (uint64_t)isr59);
	set_idt_gate(60, (uint64_t)isr60);
	set_idt_gate(61, (uint64_t)isr61);
	set_idt_gate(62, (uint64_t)isr62);
	set_idt_gate(63, (uint64_t)isr63);
	set_idt_gate(64, (uint64_t)isr64);
	set_idt_gate(65, (uint64_t)isr65);
	set_idt_gate(66, (uint64_t)isr66);
	set_idt_gate(67, (uint64_t)isr67);
	set_idt_gate(68, (uint64_t)isr68);
	set_idt_gate(69, (uint64_t)isr69);
	set_idt_gate(70, (uint64_t)isr70);
	set_idt_gate(71, (uint64_t)isr71);
	set_idt_gate(72, (uint64_t)isr72);
	set_idt_gate(73, (uint64_t)isr73);
	set_idt_gate(74, (uint64_t)isr74);
	set_idt_gate(75, (uint64_t)isr75);
	set_idt_gate(76, (uint64_t)isr76);
	set_idt_gate(77, (uint64_t)isr77);
	set_idt_gate(78, (uint64_t)isr78);
	set_idt_gate(79, (uint64_t)isr79);
	set_idt_gate(80, (uint64_t)isr80);
	set_idt_gate(81, (uint64_t)isr81);
	set_idt_gate(82, (uint64_t)isr82);
	set_idt_gate(83, (uint64_t)isr83);
	set_idt_gate(84, (uint64_t)isr84);
	set_idt_gate(85, (uint64_t)isr85);
	set_idt_gate(86, (uint64_t)isr86);
	set_idt_gate(87, (uint64_t)isr87);
	set_idt_gate(88, (uint64_t)isr88);
	set_idt_gate(89, (uint64_t)isr89);
	set_idt_gate(90, (uint64_t)isr90);
	set_idt_gate(91, (uint64_t)isr91);
	set_idt_gate(92, (uint64_t)isr92);
	set_idt_gate(93, (uint64_t)isr93);
	set_idt_gate(94, (uint64_t)isr94);
	set_idt_gate(95, (uint64_t)isr95);
	set_idt_gate(96, (uint64_t)isr96);
	set_idt_gate(97, (uint64_t)isr97);
	set_idt_gate(98, (uint64_t)isr98);
	set_idt_gate(99, (uint64_t)isr99);
	set_idt_gate(100, (uint64_t)isr100);
	set_idt_gate(101, (uint64_t)isr101);
	set_idt_gate(102, (uint64_t)isr102);
	set_idt_gate(103, (uint64_t)isr103);
	set_idt_gate(104, (uint64_t)isr104);
	set_idt_gate(105, (uint64_t)isr105);
	set_idt_gate(106, (uint64_t)isr106);
	set_idt_gate(107, (uint64_t)isr107);
	set_idt_gate(108, (uint64_t)isr108);
	set_idt_gate(109, (uint64_t)isr109);
	set_idt_gate(110, (uint64_t)isr110);
	set_idt_gate(111, (uint64_t)isr111);
	set_idt_gate(112, (uint64_t)isr112);
	set_idt_gate(113, (uint64_t)isr113);
	set_idt_gate(114, (uint64_t)isr114);
	set_idt_gate(115, (uint64_t)isr115);
	set_idt_gate(116, (uint64_t)isr116);
	set_idt_gate(117, (uint64_t)isr117);
	set_idt_gate(118, (uint64_t)isr118);
	set_idt_gate(119, (uint64_t)isr119);
	set_idt_gate(120, (uint64_t)isr120);
	set_idt_gate(121, (uint64_t)isr121);
	set_idt_gate(122, (uint64_t)isr122);
	set_idt_gate(123, (uint64_t)isr123);
	set_idt_gate(124, (uint64_t)isr124);
	set_idt_gate(125, (uint64_t)isr125);
	set_idt_gate(126, (uint64_t)isr126);
	set_idt_gate(127, (uint64_t)isr127);
	set_idt_gate(128, (uint64_t)isr128);
	set_idt_gate(129, (uint64_t)isr129);
	set_idt_gate(130, (uint64_t)isr130);
	set_idt_gate(131, (uint64_t)isr131);
	set_idt_gate(132, (uint64_t)isr132);
	set_idt_gate(133, (uint64_t)isr133);
	set_idt_gate(134, (uint64_t)isr134);
	set_idt_gate(135, (uint64_t)isr135);
	set_idt_gate(136, (uint64_t)isr136);
	set_idt_gate(137, (uint64_t)isr137);
	set_idt_gate(138, (uint64_t)isr138);
	set_idt_gate(139, (uint64_t)isr139);
	set_idt_gate(140, (uint64_t)isr140);
	set_idt_gate(141, (uint64_t)isr141);
	set_idt_gate(142, (uint64_t)isr142);
	set_idt_gate(143, (uint64_t)isr143);
	set_idt_gate(144, (uint64_t)isr144);
	set_idt_gate(145, (uint64_t)isr145);
	set_idt_gate(146, (uint64_t)isr146);
	set_idt_gate(147, (uint64_t)isr147);
	set_idt_gate(148, (uint64_t)isr148);
	set_idt_gate(149, (uint64_t)isr149);
	set_idt_gate(150, (uint64_t)isr150);
	set_idt_gate(151, (uint64_t)isr151);
	set_idt_gate(152, (uint64_t)isr152);
	set_idt_gate(153, (uint64_t)isr153);
	set_idt_gate(154, (uint64_t)isr154);
	set_idt_gate(155, (uint64_t)isr155);
	set_idt_gate(156, (uint64_t)isr156);
	set_idt_gate(157, (uint64_t)isr157);
	set_idt_gate(158, (uint64_t)isr158);
	set_idt_gate(159, (uint64_t)isr159);
	set_idt_gate(160, (uint64_t)isr160);
	set_idt_gate(161, (uint64_t)isr161);
	set_idt_gate(162, (uint64_t)isr162);
	set_idt_gate(163, (uint64_t)isr163);
	set_idt_gate(164, (uint64_t)isr164);
	set_idt_gate(165, (uint64_t)isr165);
	set_idt_gate(166, (uint64_t)isr166);
	set_idt_gate(167, (uint64_t)isr167);
	set_idt_gate(168, (uint64_t)isr168);
	set_idt_gate(169, (uint64_t)isr169);
	set_idt_gate(170, (uint64_t)isr170);
	set_idt_gate(171, (uint64_t)isr171);
	set_idt_gate(172, (uint64_t)isr172);
	set_idt_gate(173, (uint64_t)isr173);
	set_idt_gate(174, (uint64_t)isr174);
	set_idt_gate(175, (uint64_t)isr175);
	set_idt_gate(176, (uint64_t)isr176);
	set_idt_gate(177, (uint64_t)isr177);
	set_idt_gate(178, (uint64_t)isr178);
	set_idt_gate(179, (uint64_t)isr179);
	set_idt_gate(180, (uint64_t)isr180);
	set_idt_gate(181, (uint64_t)isr181);
	set_idt_gate(182, (uint64_t)isr182);
	set_idt_gate(183, (uint64_t)isr183);
	set_idt_gate(184, (uint64_t)isr184);
	set_idt_gate(185, (uint64_t)isr185);
	set_idt_gate(186, (uint64_t)isr186);
	set_idt_gate(187, (uint64_t)isr187);
	set_idt_gate(188, (uint64_t)isr188);
	set_idt_gate(189, (uint64_t)isr189);
	set_idt_gate(190, (uint64_t)isr190);
	set_idt_gate(191, (uint64_t)isr191);
	set_idt_gate(192, (uint64_t)isr192);
	set_idt_gate(193, (uint64_t)isr193);
	set_idt_gate(194, (uint64_t)isr194);
	set_idt_gate(195, (uint64_t)isr195);
	set_idt_gate(196, (uint64_t)isr196);
	set_idt_gate(197, (uint64_t)isr197);
	set_idt_gate(198, (uint64_t)isr198);
	set_idt_gate(199, (uint64_t)isr199);
	set_idt_gate(200, (uint64_t)isr200);
	set_idt_gate(201, (uint64_t)isr201);
	set_idt_gate(202, (uint64_t)isr202);
	set_idt_gate(203, (uint64_t)isr203);
	set_idt_gate(204, (uint64_t)isr204);
	set_idt_gate(205, (uint64_t)isr205);
	set_idt_gate(206, (uint64_t)isr206);
	set_idt_gate(207, (uint64_t)isr207);
	set_idt_gate(208, (uint64_t)isr208);
	set_idt_gate(209, (uint64_t)isr209);
	set_idt_gate(210, (uint64_t)isr210);
	set_idt_gate(211, (uint64_t)isr211);
	set_idt_gate(212, (uint64_t)isr212);
	set_idt_gate(213, (uint64_t)isr213);
	set_idt_gate(214, (uint64_t)isr214);
	set_idt_gate(215, (uint64_t)isr215);
	set_idt_gate(216, (uint64_t)isr216);
	set_idt_gate(217, (uint64_t)isr217);
	set_idt_gate(218, (uint64_t)isr218);
	set_idt_gate(219, (uint64_t)isr219);
	set_idt_gate(220, (uint64_t)isr220);
	set_idt_gate(221, (uint64_t)isr221);
	set_idt_gate(222, (uint64_t)isr222);
	set_idt_gate(223, (uint64_t)isr223);
	set_idt_gate(224, (uint64_t)isr224);
	set_idt_gate(225, (uint64_t)isr225);
	set_idt_gate(226, (uint64_t)isr226);
	set_idt_gate(227, (uint64_t)isr227);
	set_idt_gate(228, (uint64_t)isr228);
	set_idt_gate(229, (uint64_t)isr229);
	set_idt_gate(230, (uint64_t)isr230);
	set_idt_gate(231, (uint64_t)isr231);
	set_idt_gate(232, (uint64_t)isr232);
	set_idt_gate(233, (uint64_t)isr233);
	set_idt_gate(234, (uint64_t)isr234);
	set_idt_gate(235, (uint64_t)isr235);
	set_idt_gate(236, (uint64_t)isr236);
	set_idt_gate(237, (uint64_t)isr237);
	set_idt_gate(238, (uint64_t)isr238);
	set_idt_gate(239, (uint64_t)isr239);
	set_idt_gate(240, (uint64_t)isr240);
	set_idt_gate(241, (uint64_t)isr241);
	set_idt_gate(242, (uint64_t)isr242);
	set_idt_gate(243, (uint64_t)isr243);
	set_idt_gate(244, (uint64_t)isr244);
	set_idt_gate(245, (uint64_t)isr245);
	set_idt_gate(246, (uint64_t)isr246);
	set_idt_gate(247, (uint64_t)isr247);
	set_idt_gate(248, (uint64_t)isr248);
	set_idt_gate(249, (uint64_t)isr249);
	set_idt_gate(250, (uint64_t)isr250);
	set_idt_gate(251, (uint64_t)isr251);
	set_idt_gate(252, (uint64_t)isr252);
	set_idt_gate(253, (uint64_t)isr253);
	set_idt_gate(254, (uint64_t)isr254);
	set_idt_gate(255, (uint64_t)isr255);
	set_idt();
}

static const char *exceptionMessages[] = {"Divide by zero",
										  "Debug",
										  "NMI",
										  "Breakpoint",
										  "Overflow",
										  "Bound Range Exceeded",
										  "Invaild Opcode",
										  "Device Not Available",
										  "Double fault",
										  "Co-processor Segment Overrun",
										  "Invaild TSS",
										  "Segment not present",
										  "Stack-Segment Fault",
										  "GPF",
										  "Page Fault",
										  "Reserved",
										  "x87 Floating Point Exception",
										  "allignement check",
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

static eventHandlers_t eventHandlers[] = {
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

void isr_handler(registers_t *r) {
	if (r->isrNumber < 32) {
		uint64_t cr2;
		asm volatile("cli\n"
					 "mov %%cr2, %0"
					 : "=a"(cr2));
		char x[72];
		sprintf(x, "System Service Exception Not Handled: %s",
				exceptionMessages[r->isrNumber]);
		PANIC(x);
	}
	port_byte_out(0xA0, 0x20);
	port_byte_out(0x20, 0x20);
	if (eventHandlers[r->isrNumber] != NULL)
		eventHandlers[r->isrNumber](r);
}
