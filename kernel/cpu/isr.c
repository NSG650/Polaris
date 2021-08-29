/*
 * Copyright 2021 Misha
 * Copyright 2021 NSG650
 * Copyright 2021 Sebastian
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "isr.h"
#include "../kernel/panic.h"
#include "../klibc/printf.h"
#include "apic.h"
#include "idt.h"

void isr_install(void) {
	set_idt_gate(0, isr0);
	set_idt_gate(1, isr1);
	set_idt_gate(2, isr2);
	set_idt_gate(3, isr3);
	set_idt_gate(4, isr4);
	set_idt_gate(5, isr5);
	set_idt_gate(6, isr6);
	set_idt_gate(7, isr7);
	set_idt_gate(8, errorIsr8);
	set_idt_gate(9, isr9);
	set_idt_gate(10, errorIsr10);
	set_idt_gate(11, errorIsr11);
	set_idt_gate(12, errorIsr12);
	set_idt_gate(13, errorIsr13);
	set_idt_gate(14, errorIsr14);
	set_idt_gate(15, isr15);
	set_idt_gate(16, isr16);
	set_idt_gate(17, isr17);
	set_idt_gate(18, isr18);
	set_idt_gate(19, isr19);
	set_idt_gate(20, isr20);
	set_idt_gate(21, isr21);
	set_idt_gate(22, isr22);
	set_idt_gate(23, isr23);
	set_idt_gate(24, isr24);
	set_idt_gate(25, isr25);
	set_idt_gate(26, isr26);
	set_idt_gate(27, isr27);
	set_idt_gate(28, isr28);
	set_idt_gate(29, isr29);
	set_idt_gate(30, isr30);
	set_idt_gate(31, isr31);
	set_idt_gate(32, isr32);
	set_idt_gate(33, isr33);
	set_idt_gate(34, isr34);
	set_idt_gate(35, isr35);
	set_idt_gate(36, isr36);
	set_idt_gate(37, isr37);
	set_idt_gate(38, isr38);
	set_idt_gate(39, isr39);
	set_idt_gate(40, isr40);
	set_idt_gate(41, isr41);
	set_idt_gate(42, isr42);
	set_idt_gate(43, isr43);
	set_idt_gate(44, isr44);
	set_idt_gate(45, isr45);
	set_idt_gate(46, isr46);
	set_idt_gate(47, isr47);
	set_idt_gate(48, isr48);
	set_idt_gate(49, isr49);
	set_idt_gate(50, isr50);
	set_idt_gate(51, isr51);
	set_idt_gate(52, isr52);
	set_idt_gate(53, isr53);
	set_idt_gate(54, isr54);
	set_idt_gate(55, isr55);
	set_idt_gate(56, isr56);
	set_idt_gate(57, isr57);
	set_idt_gate(58, isr58);
	set_idt_gate(59, isr59);
	set_idt_gate(60, isr60);
	set_idt_gate(61, isr61);
	set_idt_gate(62, isr62);
	set_idt_gate(63, isr63);
	set_idt_gate(64, isr64);
	set_idt_gate(65, isr65);
	set_idt_gate(66, isr66);
	set_idt_gate(67, isr67);
	set_idt_gate(68, isr68);
	set_idt_gate(69, isr69);
	set_idt_gate(70, isr70);
	set_idt_gate(71, isr71);
	set_idt_gate(72, isr72);
	set_idt_gate(73, isr73);
	set_idt_gate(74, isr74);
	set_idt_gate(75, isr75);
	set_idt_gate(76, isr76);
	set_idt_gate(77, isr77);
	set_idt_gate(78, isr78);
	set_idt_gate(79, isr79);
	set_idt_gate(80, isr80);
	set_idt_gate(81, isr81);
	set_idt_gate(82, isr82);
	set_idt_gate(83, isr83);
	set_idt_gate(84, isr84);
	set_idt_gate(85, isr85);
	set_idt_gate(86, isr86);
	set_idt_gate(87, isr87);
	set_idt_gate(88, isr88);
	set_idt_gate(89, isr89);
	set_idt_gate(90, isr90);
	set_idt_gate(91, isr91);
	set_idt_gate(92, isr92);
	set_idt_gate(93, isr93);
	set_idt_gate(94, isr94);
	set_idt_gate(95, isr95);
	set_idt_gate(96, isr96);
	set_idt_gate(97, isr97);
	set_idt_gate(98, isr98);
	set_idt_gate(99, isr99);
	set_idt_gate(100, isr100);
	set_idt_gate(101, isr101);
	set_idt_gate(102, isr102);
	set_idt_gate(103, isr103);
	set_idt_gate(104, isr104);
	set_idt_gate(105, isr105);
	set_idt_gate(106, isr106);
	set_idt_gate(107, isr107);
	set_idt_gate(108, isr108);
	set_idt_gate(109, isr109);
	set_idt_gate(110, isr110);
	set_idt_gate(111, isr111);
	set_idt_gate(112, isr112);
	set_idt_gate(113, isr113);
	set_idt_gate(114, isr114);
	set_idt_gate(115, isr115);
	set_idt_gate(116, isr116);
	set_idt_gate(117, isr117);
	set_idt_gate(118, isr118);
	set_idt_gate(119, isr119);
	set_idt_gate(120, isr120);
	set_idt_gate(121, isr121);
	set_idt_gate(122, isr122);
	set_idt_gate(123, isr123);
	set_idt_gate(124, isr124);
	set_idt_gate(125, isr125);
	set_idt_gate(126, isr126);
	set_idt_gate(127, isr127);
	set_idt_gate(128, isr128);
	set_idt_gate(129, isr129);
	set_idt_gate(130, isr130);
	set_idt_gate(131, isr131);
	set_idt_gate(132, isr132);
	set_idt_gate(133, isr133);
	set_idt_gate(134, isr134);
	set_idt_gate(135, isr135);
	set_idt_gate(136, isr136);
	set_idt_gate(137, isr137);
	set_idt_gate(138, isr138);
	set_idt_gate(139, isr139);
	set_idt_gate(140, isr140);
	set_idt_gate(141, isr141);
	set_idt_gate(142, isr142);
	set_idt_gate(143, isr143);
	set_idt_gate(144, isr144);
	set_idt_gate(145, isr145);
	set_idt_gate(146, isr146);
	set_idt_gate(147, isr147);
	set_idt_gate(148, isr148);
	set_idt_gate(149, isr149);
	set_idt_gate(150, isr150);
	set_idt_gate(151, isr151);
	set_idt_gate(152, isr152);
	set_idt_gate(153, isr153);
	set_idt_gate(154, isr154);
	set_idt_gate(155, isr155);
	set_idt_gate(156, isr156);
	set_idt_gate(157, isr157);
	set_idt_gate(158, isr158);
	set_idt_gate(159, isr159);
	set_idt_gate(160, isr160);
	set_idt_gate(161, isr161);
	set_idt_gate(162, isr162);
	set_idt_gate(163, isr163);
	set_idt_gate(164, isr164);
	set_idt_gate(165, isr165);
	set_idt_gate(166, isr166);
	set_idt_gate(167, isr167);
	set_idt_gate(168, isr168);
	set_idt_gate(169, isr169);
	set_idt_gate(170, isr170);
	set_idt_gate(171, isr171);
	set_idt_gate(172, isr172);
	set_idt_gate(173, isr173);
	set_idt_gate(174, isr174);
	set_idt_gate(175, isr175);
	set_idt_gate(176, isr176);
	set_idt_gate(177, isr177);
	set_idt_gate(178, isr178);
	set_idt_gate(179, isr179);
	set_idt_gate(180, isr180);
	set_idt_gate(181, isr181);
	set_idt_gate(182, isr182);
	set_idt_gate(183, isr183);
	set_idt_gate(184, isr184);
	set_idt_gate(185, isr185);
	set_idt_gate(186, isr186);
	set_idt_gate(187, isr187);
	set_idt_gate(188, isr188);
	set_idt_gate(189, isr189);
	set_idt_gate(190, isr190);
	set_idt_gate(191, isr191);
	set_idt_gate(192, isr192);
	set_idt_gate(193, isr193);
	set_idt_gate(194, isr194);
	set_idt_gate(195, isr195);
	set_idt_gate(196, isr196);
	set_idt_gate(197, isr197);
	set_idt_gate(198, isr198);
	set_idt_gate(199, isr199);
	set_idt_gate(200, isr200);
	set_idt_gate(201, isr201);
	set_idt_gate(202, isr202);
	set_idt_gate(203, isr203);
	set_idt_gate(204, isr204);
	set_idt_gate(205, isr205);
	set_idt_gate(206, isr206);
	set_idt_gate(207, isr207);
	set_idt_gate(208, isr208);
	set_idt_gate(209, isr209);
	set_idt_gate(210, isr210);
	set_idt_gate(211, isr211);
	set_idt_gate(212, isr212);
	set_idt_gate(213, isr213);
	set_idt_gate(214, isr214);
	set_idt_gate(215, isr215);
	set_idt_gate(216, isr216);
	set_idt_gate(217, isr217);
	set_idt_gate(218, isr218);
	set_idt_gate(219, isr219);
	set_idt_gate(220, isr220);
	set_idt_gate(221, isr221);
	set_idt_gate(222, isr222);
	set_idt_gate(223, isr223);
	set_idt_gate(224, isr224);
	set_idt_gate(225, isr225);
	set_idt_gate(226, isr226);
	set_idt_gate(227, isr227);
	set_idt_gate(228, isr228);
	set_idt_gate(229, isr229);
	set_idt_gate(230, isr230);
	set_idt_gate(231, isr231);
	set_idt_gate(232, isr232);
	set_idt_gate(233, isr233);
	set_idt_gate(234, isr234);
	set_idt_gate(235, isr235);
	set_idt_gate(236, isr236);
	set_idt_gate(237, isr237);
	set_idt_gate(238, isr238);
	set_idt_gate(239, isr239);
	set_idt_gate(240, isr240);
	set_idt_gate(241, isr241);
	set_idt_gate(242, isr242);
	set_idt_gate(243, isr243);
	set_idt_gate(244, isr244);
	set_idt_gate(245, isr245);
	set_idt_gate(246, isr246);
	set_idt_gate(247, isr247);
	set_idt_gate(248, isr248);
	set_idt_gate(249, isr249);
	set_idt_gate(250, isr250);
	set_idt_gate(251, isr251);
	set_idt_gate(252, isr252);
	set_idt_gate(253, isr253);
	set_idt_gate(254, isr254);
	set_idt_gate(255, isr255);
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

static eventHandlers_t eventHandlers[256] = {NULL};

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
		__builtin_unreachable();
	}
	apic_eoi();
	if (eventHandlers[r->isrNumber] != NULL)
		eventHandlers[r->isrNumber](r);
}

void isr_register_handler(int n, void *handler) {
	eventHandlers[n] = handler;
}
