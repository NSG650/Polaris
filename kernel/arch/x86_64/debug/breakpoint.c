#include <debug/debug.h>
#include <klibc/mem.h>
#include <sys/isr.h>
#include <zydis/Zydis.h>

#define DUMP_SIZE 100

void breakpoint_handler(registers_t *reg) {
	kprintffos(0, "=========== Start of dumps =========\n");
	if (reg->cs & 0x3) {
		kprintffos(0, "Breakpoint hit in user!\n");
	} else {
		kprintffos(0, "Breakpoint hit in kernel!\n");
	}
	kprintffos(0, "========= Register dumps =========\n");
	kprintffos(0, "RIP: 0x%p RBP: 0x%p RSP: 0x%p\n", reg->rip, reg->rbp,
			   reg->rsp);
	kprintffos(0, "RAX: 0x%p RBX: 0x%p RCX: 0x%p\n", reg->rax, reg->rbx,
			   reg->rcx);
	kprintffos(0, "RDX: 0x%p RDI: 0x%p RSI: 0x%p\n", reg->rdx, reg->rdi,
			   reg->rsi);
	kprintffos(0, "R8 : 0x%p R9 : 0x%p R10: 0x%p\n", reg->r8, reg->r9,
			   reg->r10);
	kprintffos(0, "R11: 0x%p R12: 0x%p R13: 0x%p\n", reg->r11, reg->r12,
			   reg->r13);
	kprintffos(0, "R14: 0x%p R15: 0x%p\n", reg->r14, reg->r15);
	kprintffos(0, "CS : 0x%p SS : 0x%p RFLAGS: 0x%p\n", reg->cs, reg->ss,
			   reg->rflags);
	kprintffos(0, "============ Code dump ===========\n");
	// dump the code present at rip
	uint8_t *code = (uint8_t *)reg->rip;
	uint8_t code_dis[DUMP_SIZE] = {0};
	for (size_t i = 0; i < DUMP_SIZE; i++) {
		kprintffos(0, "\\x%x", code[i]);
	}
	memcpy(code_dis, code, 100);
	kprintffos(0, "\n============ Disassembly ===========\n");

	// Just the example code from zydis

	ZydisDecoder decoder;
	ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64,
					 ZYDIS_STACK_WIDTH_64);

	ZydisFormatter formatter;
	ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);

	ZyanU64 runtime_address = reg->rip;
	ZyanUSize offset = 0;
	const ZyanUSize length = sizeof(code_dis);
	ZydisDecodedInstruction instruction;
	ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT_VISIBLE];

	while (ZYAN_SUCCESS(ZydisDecoderDecodeFull(
		&decoder, code_dis + offset, length - offset, &instruction, operands,
		ZYDIS_MAX_OPERAND_COUNT_VISIBLE, ZYDIS_DFLAG_VISIBLE_OPERANDS_ONLY))) {
		// Format & print the binary instruction structure to human readable
		// format
		char buffer[256];
		ZydisFormatterFormatInstruction(&formatter, &instruction, operands,
										instruction.operand_count_visible,
										buffer, sizeof(buffer), runtime_address,
										ZYAN_NULL);

		kprintffos(0, "0x%p: %s\n", runtime_address, buffer);
		offset += instruction.length;
		runtime_address += instruction.length;
	}

	kprintffos(0, "============ End of dumps ==========\n");
}