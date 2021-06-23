#ifndef ISR_H
#define ISR_H

#include <stdint.h>
#include <stddef.h>
#include "idt.h"
#include "reg.h"

void isr1();
void isr2();
void isr0();
void isr3();
void isr4();
void isr5();
void isr6();
void isr7();
void errorIsr8();
void isr9();
void errorIsr10();
void errorIsr11();
void errorIsr12();
void errorIsr13();
void errorIsr14();
void isr15();
void isr16();
void isr17();
void isr18();
void isr19();
void isr20();
void isr21();
void isr22();
void isr23();
void isr24();
void isr25();
void isr26();
void isr27();
void isr28();
void isr29();
void isr30();
void isr31();
void isr32();
void isr33();
void isr34();
void isr35();
void isr36();
void isr37();
void isr38();
void isr39();
void isr40();
void isr41();
void isr42();
void isr43();
void isr44();
void isr45();
void isr46();
void isr47();
void isr48();
void isr49();
void isr50();
void isr51();
void isr52();
void isr53();
void isr54();
void isr55();
void isr56();
void isr57();
void isr58();
void isr59();
void isr60();
void isr61();
void isr62();
void isr63();
void isr64();
void isr65();
void isr66();
void isr67();
void isr68();
void isr69();
void isr70();
void isr71();
void isr72();
void isr73();
void isr74();
void isr75();
void isr76();
void isr77();
void isr78();
void isr79();
void isr80();
void isr81();
void isr82();
void isr83();
void isr84();
void isr85();
void isr86();
void isr87();
void isr88();
void isr89();
void isr90();
void isr91();
void isr92();
void isr93();
void isr94();
void isr95();
void isr96();
void isr97();
void isr98();
void isr99();
void isr100();
void isr101();
void isr102();
void isr103();
void isr104();
void isr105();
void isr106();
void isr107();
void isr108();
void isr109();
void isr110();
void isr111();
void isr112();
void isr113();
void isr114();
void isr115();
void isr116();
void isr117();
void isr118();
void isr119();
void isr120();
void isr121();
void isr122();
void isr123();
void isr124();
void isr125();
void isr126();
void isr127();
void isr128();
void isr129();
void isr130();
void isr131();
void isr132();
void isr133();
void isr134();
void isr135();
void isr136();
void isr137();
void isr138();
void isr139();
void isr140();
void isr141();
void isr142();
void isr143();
void isr144();
void isr145();
void isr146();
void isr147();
void isr148();
void isr149();
void isr150();
void isr151();
void isr152();
void isr153();
void isr154();
void isr155();
void isr156();
void isr157();
void isr158();
void isr159();
void isr160();
void isr161();
void isr162();
void isr163();
void isr164();
void isr165();
void isr166();
void isr167();
void isr168();
void isr169();
void isr170();
void isr171();
void isr172();
void isr173();
void isr174();
void isr175();
void isr176();
void isr177();
void isr178();
void isr179();
void isr180();
void isr181();
void isr182();
void isr183();
void isr184();
void isr185();
void isr186();
void isr187();
void isr188();
void isr189();
void isr190();
void isr191();
void isr192();
void isr193();
void isr194();
void isr195();
void isr196();
void isr197();
void isr198();
void isr199();
void isr200();
void isr201();
void isr202();
void isr203();
void isr204();
void isr205();
void isr206();
void isr207();
void isr208();
void isr209();
void isr210();
void isr211();
void isr212();
void isr213();
void isr214();
void isr215();
void isr216();
void isr217();
void isr218();
void isr219();
void isr220();
void isr221();
void isr222();
void isr223();
void isr224();
void isr225();
void isr226();
void isr227();
void isr228();
void isr229();
void isr230();
void isr231();
void isr232();
void isr233();
void isr234();
void isr235();
void isr236();
void isr237();
void isr238();
void isr239();
void isr240();
void isr241();
void isr242();
void isr243();
void isr244();
void isr245();
void isr246();
void isr247();
void isr248();
void isr249();
void isr250();
void isr251();
void isr252();
void isr253();
void isr254();
void isr255();

typedef void (*eventHandlers_t)(registers_t*);

inline const char *exceptionMessages[] = {"Divide by zero",
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

void isr_install();
extern void isr_handler(registers_t *r);

inline eventHandlers_t eventHandlers[] =   {
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
                                    };

#endif