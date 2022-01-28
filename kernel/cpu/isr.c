/*
 * Copyright 2021, 2022 Misha
 * Copyright 2021, 2022 NSG650
 * Copyright 2021, 2022 Sebastian
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
#include "../mm/vmm.h"
#include "../sys/gdt.h"
#include "apic.h"
#include "idt.h"
#include <liballoc.h>

void isr_install(void) {
	// Set up IST 1 for severe exceptions
	struct tss *tss_desc = kmalloc(sizeof(struct tss));
	tss_desc->ist1 = (uint64_t)kmalloc(32768) + 32768;
	gdt_load_tss((size_t)tss_desc);
	// Set up ISR handlers
	set_idt_gate(0, isr0, 0);
	set_idt_gate(1, isr1, 1);
	set_idt_gate(2, isr2, 1);
	set_idt_gate(3, isr3, 0);
	set_idt_gate(4, isr4, 0);
	set_idt_gate(5, isr5, 0);
	set_idt_gate(6, isr6, 0);
	set_idt_gate(7, isr7, 0);
	set_idt_gate(8, isr8, 1);
	set_idt_gate(9, isr9, 0);
	set_idt_gate(10, isr10, 0);
	set_idt_gate(11, isr11, 0);
	set_idt_gate(12, isr12, 0);
	set_idt_gate(13, isr13, 0);
	set_idt_gate(14, isr14, 0);
	set_idt_gate(15, isr15, 0);
	set_idt_gate(16, isr16, 0);
	set_idt_gate(17, isr17, 0);
	set_idt_gate(18, isr18, 1);
	set_idt_gate(19, isr19, 0);
	set_idt_gate(20, isr20, 0);
	set_idt_gate(21, isr21, 0);
	set_idt_gate(22, isr22, 0);
	set_idt_gate(23, isr23, 0);
	set_idt_gate(24, isr24, 0);
	set_idt_gate(25, isr25, 0);
	set_idt_gate(26, isr26, 0);
	set_idt_gate(27, isr27, 0);
	set_idt_gate(28, isr28, 0);
	set_idt_gate(29, isr29, 0);
	set_idt_gate(30, isr30, 0);
	set_idt_gate(31, isr31, 0);
	set_idt_gate(32, isr32, 0);
	set_idt_gate(33, isr33, 0);
	set_idt_gate(34, isr34, 0);
	set_idt_gate(35, isr35, 0);
	set_idt_gate(36, isr36, 0);
	set_idt_gate(37, isr37, 0);
	set_idt_gate(38, isr38, 0);
	set_idt_gate(39, isr39, 0);
	set_idt_gate(40, isr40, 0);
	set_idt_gate(41, isr41, 0);
	set_idt_gate(42, isr42, 0);
	set_idt_gate(43, isr43, 0);
	set_idt_gate(44, isr44, 0);
	set_idt_gate(45, isr45, 0);
	set_idt_gate(46, isr46, 0);
	set_idt_gate(47, isr47, 0);
	set_idt_gate(48, isr48, 0);
	set_idt_gate(49, isr49, 0);
	set_idt_gate(50, isr50, 0);
	set_idt_gate(51, isr51, 0);
	set_idt_gate(52, isr52, 0);
	set_idt_gate(53, isr53, 0);
	set_idt_gate(54, isr54, 0);
	set_idt_gate(55, isr55, 0);
	set_idt_gate(56, isr56, 0);
	set_idt_gate(57, isr57, 0);
	set_idt_gate(58, isr58, 0);
	set_idt_gate(59, isr59, 0);
	set_idt_gate(60, isr60, 0);
	set_idt_gate(61, isr61, 0);
	set_idt_gate(62, isr62, 0);
	set_idt_gate(63, isr63, 0);
	set_idt_gate(64, isr64, 0);
	set_idt_gate(65, isr65, 0);
	set_idt_gate(66, isr66, 0);
	set_idt_gate(67, isr67, 0);
	set_idt_gate(68, isr68, 0);
	set_idt_gate(69, isr69, 0);
	set_idt_gate(70, isr70, 0);
	set_idt_gate(71, isr71, 0);
	set_idt_gate(72, isr72, 0);
	set_idt_gate(73, isr73, 0);
	set_idt_gate(74, isr74, 0);
	set_idt_gate(75, isr75, 0);
	set_idt_gate(76, isr76, 0);
	set_idt_gate(77, isr77, 0);
	set_idt_gate(78, isr78, 0);
	set_idt_gate(79, isr79, 0);
	set_idt_gate(80, isr80, 0);
	set_idt_gate(81, isr81, 0);
	set_idt_gate(82, isr82, 0);
	set_idt_gate(83, isr83, 0);
	set_idt_gate(84, isr84, 0);
	set_idt_gate(85, isr85, 0);
	set_idt_gate(86, isr86, 0);
	set_idt_gate(87, isr87, 0);
	set_idt_gate(88, isr88, 0);
	set_idt_gate(89, isr89, 0);
	set_idt_gate(90, isr90, 0);
	set_idt_gate(91, isr91, 0);
	set_idt_gate(92, isr92, 0);
	set_idt_gate(93, isr93, 0);
	set_idt_gate(94, isr94, 0);
	set_idt_gate(95, isr95, 0);
	set_idt_gate(96, isr96, 0);
	set_idt_gate(97, isr97, 0);
	set_idt_gate(98, isr98, 0);
	set_idt_gate(99, isr99, 0);
	set_idt_gate(100, isr100, 0);
	set_idt_gate(101, isr101, 0);
	set_idt_gate(102, isr102, 0);
	set_idt_gate(103, isr103, 0);
	set_idt_gate(104, isr104, 0);
	set_idt_gate(105, isr105, 0);
	set_idt_gate(106, isr106, 0);
	set_idt_gate(107, isr107, 0);
	set_idt_gate(108, isr108, 0);
	set_idt_gate(109, isr109, 0);
	set_idt_gate(110, isr110, 0);
	set_idt_gate(111, isr111, 0);
	set_idt_gate(112, isr112, 0);
	set_idt_gate(113, isr113, 0);
	set_idt_gate(114, isr114, 0);
	set_idt_gate(115, isr115, 0);
	set_idt_gate(116, isr116, 0);
	set_idt_gate(117, isr117, 0);
	set_idt_gate(118, isr118, 0);
	set_idt_gate(119, isr119, 0);
	set_idt_gate(120, isr120, 0);
	set_idt_gate(121, isr121, 0);
	set_idt_gate(122, isr122, 0);
	set_idt_gate(123, isr123, 0);
	set_idt_gate(124, isr124, 0);
	set_idt_gate(125, isr125, 0);
	set_idt_gate(126, isr126, 0);
	set_idt_gate(127, isr127, 0);
	set_idt_gate(128, isr128, 0);
	set_idt_gate(129, isr129, 0);
	set_idt_gate(130, isr130, 0);
	set_idt_gate(131, isr131, 0);
	set_idt_gate(132, isr132, 0);
	set_idt_gate(133, isr133, 0);
	set_idt_gate(134, isr134, 0);
	set_idt_gate(135, isr135, 0);
	set_idt_gate(136, isr136, 0);
	set_idt_gate(137, isr137, 0);
	set_idt_gate(138, isr138, 0);
	set_idt_gate(139, isr139, 0);
	set_idt_gate(140, isr140, 0);
	set_idt_gate(141, isr141, 0);
	set_idt_gate(142, isr142, 0);
	set_idt_gate(143, isr143, 0);
	set_idt_gate(144, isr144, 0);
	set_idt_gate(145, isr145, 0);
	set_idt_gate(146, isr146, 0);
	set_idt_gate(147, isr147, 0);
	set_idt_gate(148, isr148, 0);
	set_idt_gate(149, isr149, 0);
	set_idt_gate(150, isr150, 0);
	set_idt_gate(151, isr151, 0);
	set_idt_gate(152, isr152, 0);
	set_idt_gate(153, isr153, 0);
	set_idt_gate(154, isr154, 0);
	set_idt_gate(155, isr155, 0);
	set_idt_gate(156, isr156, 0);
	set_idt_gate(157, isr157, 0);
	set_idt_gate(158, isr158, 0);
	set_idt_gate(159, isr159, 0);
	set_idt_gate(160, isr160, 0);
	set_idt_gate(161, isr161, 0);
	set_idt_gate(162, isr162, 0);
	set_idt_gate(163, isr163, 0);
	set_idt_gate(164, isr164, 0);
	set_idt_gate(165, isr165, 0);
	set_idt_gate(166, isr166, 0);
	set_idt_gate(167, isr167, 0);
	set_idt_gate(168, isr168, 0);
	set_idt_gate(169, isr169, 0);
	set_idt_gate(170, isr170, 0);
	set_idt_gate(171, isr171, 0);
	set_idt_gate(172, isr172, 0);
	set_idt_gate(173, isr173, 0);
	set_idt_gate(174, isr174, 0);
	set_idt_gate(175, isr175, 0);
	set_idt_gate(176, isr176, 0);
	set_idt_gate(177, isr177, 0);
	set_idt_gate(178, isr178, 0);
	set_idt_gate(179, isr179, 0);
	set_idt_gate(180, isr180, 0);
	set_idt_gate(181, isr181, 0);
	set_idt_gate(182, isr182, 0);
	set_idt_gate(183, isr183, 0);
	set_idt_gate(184, isr184, 0);
	set_idt_gate(185, isr185, 0);
	set_idt_gate(186, isr186, 0);
	set_idt_gate(187, isr187, 0);
	set_idt_gate(188, isr188, 0);
	set_idt_gate(189, isr189, 0);
	set_idt_gate(190, isr190, 0);
	set_idt_gate(191, isr191, 0);
	set_idt_gate(192, isr192, 0);
	set_idt_gate(193, isr193, 0);
	set_idt_gate(194, isr194, 0);
	set_idt_gate(195, isr195, 0);
	set_idt_gate(196, isr196, 0);
	set_idt_gate(197, isr197, 0);
	set_idt_gate(198, isr198, 0);
	set_idt_gate(199, isr199, 0);
	set_idt_gate(200, isr200, 0);
	set_idt_gate(201, isr201, 0);
	set_idt_gate(202, isr202, 0);
	set_idt_gate(203, isr203, 0);
	set_idt_gate(204, isr204, 0);
	set_idt_gate(205, isr205, 0);
	set_idt_gate(206, isr206, 0);
	set_idt_gate(207, isr207, 0);
	set_idt_gate(208, isr208, 0);
	set_idt_gate(209, isr209, 0);
	set_idt_gate(210, isr210, 0);
	set_idt_gate(211, isr211, 0);
	set_idt_gate(212, isr212, 0);
	set_idt_gate(213, isr213, 0);
	set_idt_gate(214, isr214, 0);
	set_idt_gate(215, isr215, 0);
	set_idt_gate(216, isr216, 0);
	set_idt_gate(217, isr217, 0);
	set_idt_gate(218, isr218, 0);
	set_idt_gate(219, isr219, 0);
	set_idt_gate(220, isr220, 0);
	set_idt_gate(221, isr221, 0);
	set_idt_gate(222, isr222, 0);
	set_idt_gate(223, isr223, 0);
	set_idt_gate(224, isr224, 0);
	set_idt_gate(225, isr225, 0);
	set_idt_gate(226, isr226, 0);
	set_idt_gate(227, isr227, 0);
	set_idt_gate(228, isr228, 0);
	set_idt_gate(229, isr229, 0);
	set_idt_gate(230, isr230, 0);
	set_idt_gate(231, isr231, 0);
	set_idt_gate(232, isr232, 0);
	set_idt_gate(233, isr233, 0);
	set_idt_gate(234, isr234, 0);
	set_idt_gate(235, isr235, 0);
	set_idt_gate(236, isr236, 0);
	set_idt_gate(237, isr237, 0);
	set_idt_gate(238, isr238, 0);
	set_idt_gate(239, isr239, 0);
	set_idt_gate(240, isr240, 0);
	set_idt_gate(241, isr241, 0);
	set_idt_gate(242, isr242, 0);
	set_idt_gate(243, isr243, 0);
	set_idt_gate(244, isr244, 0);
	set_idt_gate(245, isr245, 0);
	set_idt_gate(246, isr246, 0);
	set_idt_gate(247, isr247, 0);
	set_idt_gate(248, isr248, 0);
	set_idt_gate(249, isr249, 0);
	set_idt_gate(250, isr250, 0);
	set_idt_gate(251, isr251, 0);
	set_idt_gate(252, isr252, 0);
	set_idt_gate(253, isr253, 0);
	set_idt_gate(254, isr254, 0);
	set_idt_gate(255, isr255, 0);
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
		if (r->isrNumber == 14)
			vmm_page_fault_handler(r);
		char x[72];
		sprintf(x, "System Service Exception Not Handled: %s",
				exceptionMessages[r->isrNumber]);
		PANIC(x);
		__builtin_unreachable();
	}
	if (eventHandlers[r->isrNumber] != NULL)
		eventHandlers[r->isrNumber](r);
	apic_eoi();
}

void isr_register_handler(int n, void *handler) {
	eventHandlers[n] = handler;
}
