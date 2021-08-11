ENTRY(_start)

SECTIONS {
	. = 0xFFFFFFFF80200000;

	/* Name required by spec */
	.stivale2hdr : ALIGN(4K) {
		KEEP(*(.stivale2hdr))
	}

	/* Actual kernel */
	.text : ALIGN(4K) {
		*(.text*)
	}

	.rodata : ALIGN(4K) {
		*(.rodata*)
	}

	.data : ALIGN(4K) {
		*(.data*)
	}

	.bss : ALIGN(4K) {
		*(COMMON)
		*(.bss*)
	}
}
