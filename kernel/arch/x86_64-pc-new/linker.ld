/* Tell the linker that we want an x86_64 ELF64 output file */
OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

/* We want the symbol arch_entry to be our entry point */
ENTRY(arch_entry)

/* Define the program headers we want so the bootloader gives us the right */
/* MMU permissions */
PHDRS {
	text    PT_LOAD    FLAGS((1 << 0) | (1 << 2)) ; /* Execute + Read */
	rodata  PT_LOAD    FLAGS((1 << 2)) ;            /* Read only */
	data    PT_LOAD    FLAGS((1 << 1) | (1 << 2)) ; /* Write + Read */
	dynamic PT_DYNAMIC FLAGS((1 << 1) | (1 << 2)) ; /* Dynamic segment needed for PIE */
}

SECTIONS {
	/* We wanna be placed in the topmost 2GiB of the address space, for optimisations */
	/* and because that is what the Limine spec mandates. */
	/* Any address in this region will do, but often 0xffffffff80000000 is chosen as */
	/* that is the beginning of the region. */
	. = 0xffffffff80000000;

	text_start_addr = .;

	.text : {
		*(.text .text.*)
	} :text

	text_end_addr = .;

	/* Move to the next memory page for .rodata */
	. += CONSTANT(MAXPAGESIZE);

	rodata_start_addr = .;

	.rodata : {
		*(.rodata .rodata.*)
	} :rodata

	rodata_end_addr = .;

	/* Move to the next memory page for .data */
	. += CONSTANT(MAXPAGESIZE);

	data_start_addr = .;

	.data : {
		*(.data .data.*)
	} :data

	.dynamic : {
		*(.dynamic)
	} :data :dynamic

	.bss : {
		*(.bss .bss.*)
		*(COMMON)
	} :data

	data_end_addr = .;

	/* Discard notes and eh_frame since they may cause issues on some hosts. */
	/DISCARD/ : {
		*(.eh_frame)
		*(.note .note.*)
	}
}
