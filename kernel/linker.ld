OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(_start)

PHDRS {
	null    PT_NULL    FLAGS(0);
	text    PT_LOAD    FLAGS((1 << 0) | (1 << 2)); /* Execute + Read */
	rodata  PT_LOAD    FLAGS((1 << 2));            /* Read only */
	data    PT_LOAD    FLAGS((1 << 1) | (1 << 2)); /* Write + Read */
}

SECTIONS {
	. = 0xFFFFFFFF80000000;

	.text : {
		*(.text*)
	} :text

	. += CONSTANT(MAXPAGESIZE);

	.stivale2hdr : {
		KEEP(*(.stivale2hdr))
	} :rodata

	.rodata : {
		*(.rodata*)
	} :rodata

	. += CONSTANT(MAXPAGESIZE);

	.data : {
		*(.data*)
	} :data

	.bss : {
		*(COMMON)
		*(.bss*)
	} :data
}
