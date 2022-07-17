#include <sys/isr.h>
#include <debug/debug.h>

#define DUMP_SIZE 100

void breakpoint_handler(registers_t *reg) {
    kprintffos(0, "=========== Start of dumps =========\n");
    if (reg->cs & 0x3) {
        kprintffos(0, "Breakpoint hit in user!\n");   
    }
    else {
        kprintffos(0, "Breakpoint hit in kernel!\n");
    }
    kprintffos(0, "========= Register dumps =========\n");
    kprintffos(0, "RIP: 0x%p RBP: 0x%p RSP: 0x%p\n", reg->rip, reg->rbp, reg->rsp);
    kprintffos(0, "RAX: 0x%p RBX: 0x%p RCX: 0x%p\n", reg->rax, reg->rbx, reg->rcx);
    kprintffos(0, "RDX: 0x%p RDI: 0x%p RSI: 0x%p\n", reg->rdx, reg->rdi, reg->rsi);    
    kprintffos(0, "R8 : 0x%p R9 : 0x%p R10: 0x%p\n", reg->r8 , reg->r9 , reg->r10);    
    kprintffos(0, "R11: 0x%p R12: 0x%p R13: 0x%p\n", reg->r11, reg->r12, reg->r13);    
    kprintffos(0, "R14: 0x%p R15: 0x%p\n", reg->r14 , reg->r15);    
    kprintffos(0, "CS : 0x%p SS : 0x%p RFLAGS: 0x%p\n", reg->cs, reg->ss, reg->rflags);
    kprintffos(0, "============ Code dump ===========\n");
    // dump the code present at rip
    uint8_t *code = (uint8_t *)reg->rip;
    for (size_t i = 0; i < DUMP_SIZE - 1; i++) {
        kprintffos(0, "\\x%x", code[i]);
    }
    kprintffos(0, "\\x%x\n", code[DUMP_SIZE - 1]);
    kprintffos(0, "============ End of dumps ==========\n");
}